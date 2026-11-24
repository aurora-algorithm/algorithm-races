// Race: wordcount (rust)
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
