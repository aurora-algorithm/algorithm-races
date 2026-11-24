// Race: bcrypt (rust)
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

const BCRYPT_COST: u32 = 10;

const REF_PASSWORD: &str = "correct horse battery staple";

const BCRYPT_REF: &str = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";

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
