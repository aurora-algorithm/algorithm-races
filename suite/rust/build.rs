use std::env;
use std::process::Command;

// Records the compiler version (e.g. "rustc 1.97.1") so the binary can name its runtime.
fn main() {
    let rustc = env::var("RUSTC").unwrap_or_else(|_| "rustc".to_string());
    let output = Command::new(rustc)
        .arg("--version")
        .output()
        .expect("rustc --version failed");
    let version = String::from_utf8_lossy(&output.stdout);
    let short: Vec<&str> = version.split_whitespace().take(2).collect();
    println!("cargo:rustc-env=RUSTC_VERSION={}", short.join(" "));
    println!("cargo:rerun-if-env-changed=RUSTC");
    println!(
        "cargo:rustc-env=WORK_CRATES={}",
        locked_versions(WORK_CRATES)
    );
    println!(
        "cargo:rustc-env=BUILTINS_CRATES={}",
        locked_versions(BUILTINS_CRATES)
    );
    println!("cargo:rerun-if-changed=Cargo.lock");
}

// The libraries of the "work" kernels, with the versions Cargo.lock pinned, for the runtime field.
const WORK_CRATES: &[&str] = &[
    "image",
    "zune-jpeg",
    "jpeg-encoder",
    "fast_image_resize",
    "flate2",
    "miniz_oxide",
    "zstd",
    "zstd-sys",
    "bcrypt",
    "argon2",
    "csv",
];

// The crates of the "builtins" group (../builtins-NOTES.md).
const BUILTINS_CRATES: &[&str] = &[
    "rayon",
    "hashbrown",
    "foldhash",
    "ahash",
    "rustc-hash",
    "voracious_radix_sort",
    "regex",
    "aho-corasick",
];

fn locked_versions(names: &[&str]) -> String {
    let lock = std::fs::read_to_string("Cargo.lock").unwrap_or_default();
    let mut found = Vec::new();
    for block in lock.split("[[package]]") {
        let field = |key: &str| {
            block
                .lines()
                .find_map(|l| l.strip_prefix(key))
                .map(|v| v.trim().trim_matches('"').to_string())
        };
        if let (Some(name), Some(version)) = (field("name = "), field("version = "))
            && names.contains(&name.as_str())
        {
            found.push(format!("{name} {version}"));
        }
    }
    found.join(", ")
}
