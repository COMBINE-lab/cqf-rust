extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=native=./c_cqf");
    println!("cargo:libdir=./c_cqf");
    println!("cargo:rustc-link-lib=static=cqf");

    let bindings = bindgen::Builder::default()
        .clang_arg("-I./c_cqf/include")
        .header("wrapper.h")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings.write_to_file(out_path.join("bindings.rs"))
        .expect("couldn't write bindings!");
}
