fn main() {
    // Tell Cargo that if the given file changes, to rerun this build script.
    println!("cargo:rerun-if-changed=src/libtorrent.h");
    // Link libtorrent-rasterbar.
    println!("cargo:rustc-link-lib=torrent-rasterbar");
    // Use lld for linking, ld might not always work.
    println!("cargo:rustc-link-arg=-fuse-ld=lld");

    // Compile libtorrent-ffi
    cc::Build::new()
        .file("src/libtorrent-ffi.cpp")
        .cpp(true)
        .compile("torrent-ffi");
    
    // Generate C++ FFI bindings for Rust
    let bindings = bindgen::Builder::default()
        .header("src/libtorrent-ffi.hpp")
        .allowlist_function("download_magnet()")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("could not generate bindings");
    
    // Output bindings.rs to src folder
    let out_path = std::path::PathBuf::from("src");
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("could not write bindings");

    // Link libtorrent-ffi statically
    println!("cargo:rustc-link-lib=static=torrent-ffi");

}
