// Race: argon2id (rust)
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

const ARGON2_M: u32 = 19456;

const ARGON2_T: u32 = 2;

const ARGON2_P: u32 = 1;

const REF_PASSWORD: &str = "correct horse battery staple";

const ARGON2_REF: &str = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

fn passwords(n: usize) -> Vec<String> {
    (0..n).map(|i| format!("user-{i}-password")).collect()
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
