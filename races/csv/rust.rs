// Race: csv (rust)
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
