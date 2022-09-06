fn main() {
    // do not run the build script if only rust code changes
    println!("cargo:rerun-if-changed=src/libtorrent-ffi.cpp");
    println!("cargo:rerun-if-changed=src/libtorrent-ffi.hpp");
    println!("cargo:rerun-if-changed=build.rs");
    // Link libtorrent-rasterbar.
    // search a directory "libs/" as alternative to being system installed
    println!("cargo:rustc-link-search=libs/");
    println!("cargo:rustc-link-lib=dylib=torrent-rasterbar");
    // Use lld for linking, ld might not always work.
    println!("cargo:rustc-link-arg=-fuse-ld=lld");

    // Compile libtorrent-ffi
    cc::Build::new()
        .cpp(true)
        .file("src/libtorrent-ffi.cpp")
        .compile("torrent-ffi");

    // Link libtorrent-ffi statically
    println!("cargo:rustc-link-lib=static=torrent-ffi");

}
