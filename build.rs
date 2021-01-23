use std::env;
use std::path::PathBuf;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    let target_dir = if let Ok(target) = env::var("CARGO_TARGET_DIR") {
        PathBuf::from(target)
    } else {
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("target")
    };

    let package_name = "br";

    // C Header
    let output_file = target_dir
        .join(format!("{}.h", package_name))
        .display()
        .to_string();

    let config = cbindgen::Config {
        language: cbindgen::Language::Cxx,
        include_guard: Some(String::from("BR_H")),
        ..Default::default()
    };
    cbindgen::generate_with_config(&crate_dir, config)
        .unwrap()
        .write_to_file(&output_file);
}
