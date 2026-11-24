use std::env;
use std::process::Command;

// Records the compiler version (e.g. "rustc 1.97.1") so the module can name its runtime.
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
    let target = env::var("TARGET").unwrap_or_default();
    println!("cargo:rustc-env=BUILD_TARGET={target}");
}
