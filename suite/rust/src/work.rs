//! The "work" kernels (../SPEC.md section 11): image resize, compression, password hashing, CSV, word count.

use std::collections::HashMap;
use std::fs::File;
use std::io::{BufRead, BufReader, Read, Write};
use std::path::{Path, PathBuf};

use flate2::Compression;
use flate2::read::GzDecoder;
use flate2::write::GzEncoder;
use image::imageops::FilterType;
use image::{DynamicImage, ImageFormat};
use jpeg_encoder::{ColorType, Encoder, SamplingFactor};
use serde::Deserialize;
use serde_json::{Value, json};

use crate::misc::sha256;
use crate::{Args, measure};

const PHOTOS: usize = 8;
const TARGET_WIDTH: u32 = 1080;
const JPEG_QUALITY: u8 = 80;
const BCRYPT_COST: u32 = 10;
const ARGON2_M: u32 = 19456;
const ARGON2_T: u32 = 2;
const ARGON2_P: u32 = 1;
const REF_PASSWORD: &str = "correct horse battery staple";
const BCRYPT_REF: &str = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";
const ARGON2_REF: &str = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

pub type Outcome = (String, Vec<f64>, Option<Value>);

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let (n, repeats) = (args.size, args.repeats);
    let outcome = match args.kernel.as_str() {
        "image" => image_kernel(args, resize_image)?,
        "image-alt" => image_kernel(args, resize_fir)?,
        "gzip" => {
            let raw = read(&args.data.join(format!("corpus-{n}.bin")))?;
            let (packed, times) = measure(repeats, || &raw, gzip);
            let check = plain_checksum(&gunzip(&packed));
            (
                check,
                times,
                Some(json!({"compressed_bytes": packed.len()})),
            )
        }
        "gunzip" => {
            let packed = gzip(&read(&args.data.join(format!("corpus-{n}.bin")))?);
            let (raw, times) = measure(repeats, || &packed, gunzip);
            (plain_checksum(&raw), times, None)
        }
        "zstd" => {
            let raw = read(&args.data.join(format!("corpus-{n}.bin")))?;
            let (packed, times) = measure(repeats, || &raw, zstd_compress);
            let check = plain_checksum(&zstd_decompress(&packed));
            (
                check,
                times,
                Some(json!({"compressed_bytes": packed.len()})),
            )
        }
        "unzstd" => {
            let packed = zstd_compress(&read(&args.data.join(format!("corpus-{n}.bin")))?);
            let (raw, times) = measure(repeats, || &packed, zstd_decompress);
            (plain_checksum(&raw), times, None)
        }
        "bcrypt" => {
            let pws = passwords(n);
            let (hashes, times) = measure(repeats, || &pws, bcrypt_hash);
            (bcrypt_checksum(&hashes), times, None)
        }
        "argon2id" => {
            let pws = passwords(n);
            let (hashes, times) = measure(repeats, || &pws, argon2_hash);
            (argon2_checksum(&hashes), times, None)
        }
        "csv" => {
            let path = args.data.join(format!("orders-{n}.csv"));
            let (totals, times) = measure(repeats, || &path, csv_totals);
            (csv_checksum(&totals?), times, None)
        }
        "wordcount" => {
            let path = args.data.join(format!("text-{n}.txt"));
            let (counts, times) = measure(repeats, || &path, wordcount);
            (wordcount_checksum(&counts?), times, None)
        }
        _ => return Ok(None),
    };
    Ok(Some(outcome))
}

pub fn is_work(kernel: &str) -> bool {
    matches!(
        kernel,
        "image"
            | "image-alt"
            | "gzip"
            | "gunzip"
            | "zstd"
            | "unzstd"
            | "bcrypt"
            | "argon2id"
            | "csv"
            | "wordcount"
    )
}

fn read(path: &Path) -> Result<Vec<u8>, String> {
    std::fs::read(path).map_err(|e| format!("cannot read {}: {e}", path.display()))
}

fn target_height(width: u32, height: u32) -> u32 {
    (height * TARGET_WIDTH + width / 2) / width
}

fn image_kernel(
    args: &Args,
    resize: fn(&DynamicImage, u32, u32) -> DynamicImage,
) -> Result<Outcome, String> {
    let photos = (0..args.size)
        .map(|i| read(&args.data.join(format!("photos/photo-{}.jpg", i % PHOTOS))))
        .collect::<Result<Vec<_>, _>>()?;
    let (result, times) = measure(args.repeats, || &photos, |p| convert_all(p, resize));
    let (width, height, out_bytes) = result?;
    let check = format!("{}:{width}x{height}", photos.len());
    Ok((check, times, Some(json!({"out_bytes": out_bytes}))))
}

fn convert_all(
    photos: &[Vec<u8>],
    resize: fn(&DynamicImage, u32, u32) -> DynamicImage,
) -> Result<(u32, u32, usize), String> {
    let (mut width, mut height, mut out_bytes) = (0, 0, 0);
    for data in photos {
        let img = image::load_from_memory_with_format(data, ImageFormat::Jpeg)
            .map_err(|e| e.to_string())?;
        let small = resize(&img, TARGET_WIDTH, target_height(img.width(), img.height()));
        let out = encode_jpeg(&small)?;
        (width, height, out_bytes) = (small.width(), small.height(), out_bytes + out.len());
    }
    Ok((width, height, out_bytes))
}

/// image 0.25's own encoder writes 4:4:4 chroma; jpeg-encoder writes 4:2:0 like every other language's library.
fn encode_jpeg(img: &DynamicImage) -> Result<Vec<u8>, String> {
    let rgb = img.as_rgb8().ok_or("the resized photo is not RGB8")?;
    let width = u16::try_from(rgb.width()).map_err(|e| e.to_string())?;
    let height = u16::try_from(rgb.height()).map_err(|e| e.to_string())?;
    let mut out = Vec::new();
    let mut encoder = Encoder::new(&mut out, JPEG_QUALITY);
    encoder.set_sampling_factor(SamplingFactor::R_4_2_0);
    encoder
        .encode(rgb.as_raw(), width, height, ColorType::Rgb)
        .map_err(|e| e.to_string())?;
    Ok(out)
}

fn resize_image(img: &DynamicImage, width: u32, height: u32) -> DynamicImage {
    img.resize_exact(width, height, FilterType::Lanczos3)
}

fn resize_fir(img: &DynamicImage, width: u32, height: u32) -> DynamicImage {
    use fast_image_resize::{FilterType, ResizeAlg, ResizeOptions, Resizer};

    let mut dst = DynamicImage::new(width, height, img.color());
    let options = ResizeOptions::new().resize_alg(ResizeAlg::Convolution(FilterType::Lanczos3));
    Resizer::new()
        .resize(img, &mut dst, &options)
        .expect("RGB8 in, RGB8 out");
    dst
}

fn plain_checksum(raw: &[u8]) -> String {
    format!("{}:{}", raw.len(), sha256(raw))
}

fn gzip(raw: &[u8]) -> Vec<u8> {
    let mut encoder = GzEncoder::new(Vec::new(), Compression::new(6));
    encoder.write_all(raw).expect("writing to a Vec");
    encoder.finish().expect("writing to a Vec")
}

fn gunzip(packed: &[u8]) -> Vec<u8> {
    let mut raw = Vec::new();
    GzDecoder::new(packed)
        .read_to_end(&mut raw)
        .expect("our own gzip stream");
    raw
}

fn zstd_compress(raw: &[u8]) -> Vec<u8> {
    zstd::bulk::compress(raw, 3).expect("compressing a slice")
}

/// One-shot decode into a buffer sized from the frame header, as the C, Java and C# rows do.
fn zstd_decompress(packed: &[u8]) -> Vec<u8> {
    let size = zstd::zstd_safe::get_frame_content_size(packed)
        .ok()
        .flatten()
        .expect("our own frame records its content size");
    let capacity = usize::try_from(size).expect("the corpus fits in memory");
    zstd::bulk::decompress(packed, capacity).expect("our own zstd frame")
}

fn passwords(n: usize) -> Vec<String> {
    (0..n).map(|i| format!("user-{i}-password")).collect()
}

fn bcrypt_hash(pws: &[String]) -> Vec<String> {
    pws.iter()
        .map(|pw| bcrypt::hash(pw, BCRYPT_COST).expect("cost 10 is valid"))
        .collect()
}

fn bcrypt_checksum(hashes: &[String]) -> String {
    let cost = format!("${BCRYPT_COST}$");
    let ok = hashes
        .iter()
        .enumerate()
        .filter(|(i, h)| {
            h.get(3..7) == Some(cost.as_str())
                && bcrypt::verify(format!("user-{i}-password"), h).unwrap_or(false)
        })
        .count();
    let reference = bcrypt::verify(REF_PASSWORD, BCRYPT_REF).unwrap_or(false);
    format!("{}:{ok}:{}", hashes.len(), u8::from(reference))
}

fn argon2() -> argon2::Argon2<'static> {
    use argon2::{Algorithm, Argon2, Params, Version};

    let params = Params::new(ARGON2_M, ARGON2_T, ARGON2_P, Some(32)).expect("OWASP parameters");
    Argon2::new(Algorithm::Argon2id, Version::V0x13, params)
}

fn argon2_hash(pws: &[String]) -> Vec<String> {
    use argon2::PasswordHasher;

    let hasher = argon2();
    pws.iter()
        .map(|pw| {
            hasher
                .hash_password(pw.as_bytes())
                .expect("hashing with a random salt")
                .to_string()
        })
        .collect()
}

fn argon2_verifies(hash: &str, password: &str) -> bool {
    use argon2::{PasswordHash, PasswordVerifier};

    PasswordHash::new(hash).is_ok_and(|parsed| {
        argon2()
            .verify_password(password.as_bytes(), &parsed)
            .is_ok()
    })
}

fn argon2_checksum(hashes: &[String]) -> String {
    let prefix = format!("$argon2id$v=19$m={ARGON2_M},t={ARGON2_T},p={ARGON2_P}$");
    let ok = hashes
        .iter()
        .enumerate()
        .filter(|(i, h)| {
            h.starts_with(&prefix) && argon2_verifies(h, &format!("user-{i}-password"))
        })
        .count();
    let reference = argon2_verifies(ARGON2_REF, REF_PASSWORD);
    format!("{}:{ok}:{}", hashes.len(), u8::from(reference))
}

#[derive(Deserialize)]
struct Order {
    customer_id: u32,
    total_cents: u64,
}

type Totals = (u64, HashMap<u32, u64>);

fn csv_totals(path: &PathBuf) -> Result<Totals, String> {
    let mut reader = csv::Reader::from_path(path).map_err(|e| e.to_string())?;
    let mut totals: HashMap<u32, u64> = HashMap::new();
    let mut rows = 0;
    for order in reader.deserialize::<Order>() {
        let order = order.map_err(|e| e.to_string())?;
        rows += 1;
        *totals.entry(order.customer_id).or_insert(0) += order.total_cents;
    }
    Ok((rows, totals))
}

fn csv_checksum((rows, totals): &Totals) -> String {
    let sum: u64 = totals.values().sum();
    let (id, top) = totals
        .iter()
        .min_by_key(|&(id, total)| (std::cmp::Reverse(*total), *id))
        .map_or((0, 0), |(id, total)| (*id, *total));
    format!("{rows}:{}:{sum}:{id}:{top}", totals.len())
}

fn wordcount(path: &PathBuf) -> Result<HashMap<String, u64>, String> {
    let file = File::open(path).map_err(|e| format!("cannot open {}: {e}", path.display()))?;
    let mut counts: HashMap<String, u64> = HashMap::new();
    for line in BufReader::new(file).lines() {
        let line = line.map_err(|e| e.to_string())?;
        for word in line.split_ascii_whitespace() {
            match counts.get_mut(word) {
                Some(count) => *count += 1,
                None => {
                    counts.insert(word.to_owned(), 1);
                }
            }
        }
    }
    Ok(counts)
}

fn wordcount_checksum(counts: &HashMap<String, u64>) -> String {
    let total: u64 = counts.values().sum();
    let (word, top) = counts
        .iter()
        .min_by_key(|&(word, count)| (std::cmp::Reverse(*count), word.as_str()))
        .map_or(("", 0), |(word, count)| (word.as_str(), *count));
    format!("{total}:{}:{word}:{top}", counts.len())
}
