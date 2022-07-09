#include "libtorrent-ffi.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

// Download provided magnet link.
// TODO: This currently leeches, should rather
// be moved to different thread and kept seeding for 
// duration of whole program.
bool download_magnet(char const* magnet) {
    // Start new libtorrent session
    lt::session session;
    // Get metadata from magnet
    lt::add_torrent_params atp = lt::parse_magnet_uri(magnet);
    // Set temporary save path
    atp.save_path = "/tmp/";
    // Initialise torrent
    lt::torrent_handle handle = session.add_torrent(atp);
    lt::torrent_status status = handle.status();

    // debug only
    printf("\nSaving file to: %s", status.save_path.c_str());

    // Until it finishes downloading, write out percentage every 10s
    bool finished = status.is_finished;
    while (!finished) {
        status = handle.status();
        printf("\n%.1f%% complete", status.progress*100);
        finished = status.is_finished;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    // Done downloading, exit libtorrent
    session.abort();
    return finished;
}
