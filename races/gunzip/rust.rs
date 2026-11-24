// Race: gunzip (rust)
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

fn read(path: &Path) -> Result<Vec<u8>, String> {
    std::fs::read(path).map_err(|e| format!("cannot read {}: {e}", path.display()))
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
