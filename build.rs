use std::process::Command;

fn main() {
    // do not run the build script if only rust code changes
    println!("cargo:rerun-if-changed=src/libtorrent-ffi.cpp");
    println!("cargo:rerun-if-changed=src/libtorrent-ffi.hpp");
    println!("cargo:rerun-if-changed=src/ffi-data.cpp");
    println!("cargo:rerun-if-changed=src/ffi-data.hpp");
    println!("cargo:rerun-if-changed=src/BittorrentClient.cpp");
    println!("cargo:rerun-if-changed=src/BittorrentClient.hpp");
    println!("cargo:rerun-if-changed=build.rs");
    // Link libtorrent-rasterbar.
    // search a directory "libs/" as alternative to being system installed
    println!("cargo:rustc-link-search=libs/");
    println!("cargo:rustc-link-lib=dylib=torrent-rasterbar");

    let mut linker: Option<String> = None;
    if let Ok(mut c) = Command::new("ld.lld").spawn() {
        linker = Some("lld".to_owned());
        let _ = c.kill();
    } else if let Ok(mut c) = Command::new("ld.mold").spawn() {
        linker = Some("mold".to_owned());
        let _ = c.kill();
    }
    if let Some(linker) = linker {
        println!("cargo:rustc-link-arg=-fuse-ld={linker}");
    }

    // Compile libtorrent-ffi
    cc::Build::new()
        .cpp(true)
        .file("src/libtorrent-ffi.cpp")
        .file("src/ffi-data.cpp")
        .file("src/BittorrentClient.cpp")
        .compile("torrent-ffi");

    // Link libtorrent-ffi statically
    println!("cargo:rustc-link-lib=static=torrent-ffi");

}
