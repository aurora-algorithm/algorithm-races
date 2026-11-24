// Race: unzstd (rust)
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
