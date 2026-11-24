// Race: image-alt (rust)
// Extracted from suite/rust/src/work.rs — build and run via suite/run.py

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

pub type Outcome = (String, Vec<f64>, Option<Value>);

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

fn resize_fir(img: &DynamicImage, width: u32, height: u32) -> DynamicImage {
    use fast_image_resize::{FilterType, ResizeAlg, ResizeOptions, Resizer};

    let mut dst = DynamicImage::new(width, height, img.color());
    let options = ResizeOptions::new().resize_alg(ResizeAlg::Convolution(FilterType::Lanczos3));
    Resizer::new()
        .resize(img, &mut dst, &options)
        .expect("RGB8 in, RGB8 out");
    dst
}
