#include "libtorrent-ffi.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>


lt::session setup_client() {
    // Start new libtorrent session
    lt::session session;
    std::cout << "Starting libtorrent version " << lt::version() << std::endl;

    // Limit connections and disable uTP
    lt::settings_pack settings = lt::default_settings();
    settings.set_int(lt::settings_pack::connections_limit, 100);
    settings.set_bool(lt::settings_pack::enable_outgoing_utp, false);
    settings.set_bool(lt::settings_pack::enable_incoming_utp, false);
    settings.set_int(lt::settings_pack::enc_policy(), 0); // unsure if actually works
    settings.set_int(lt::settings_pack::enc_level(), 3);  // unsure if actually works
    session.apply_settings(settings);

    return session;
}

void print_status(lt::torrent_handle t_handle) {
    lt::torrent_status t_status = t_handle.status();
    std::shared_ptr<const lt::torrent_info> t_info = t_handle.torrent_file();
    lt::file_storage t_files = t_info->files();

    // Print the file(s) save path
    for (int i = 0; i < t_files.num_files(); i++) {
        std::string file_path = t_status.save_path + t_files.file_path(i);
        std::cout << "Saving file " << i+1 << " to " << file_path << std::endl;
    }

    std::string states[] = { "Checking Files", "Downloading Metadata", "Downloading", "Finished",
                             "Seeding", "Allocating", "Checking Resume Data" };

    // Until it finishes downloading, poll status and write out percentage every 2s
    bool finished = t_status.is_finished;
    while (!finished) {
        t_status = t_handle.status();
        std::cout << "\r" << states[t_status.state-1] << " " << std::round(t_status.progress*100)
                  << "% " << t_status.download_rate/1000 << " KB/s down " << t_status.upload_rate/1000
                  << " KB/s up Peers:" << t_status.num_peers << "       "; // empty space for ovewriting any characters left
        std::cout << std::flush;
        finished = t_status.is_finished;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

// Download provided magnet link.
// TODO: This currently leeches, should rather
// be moved to different thread and kept seeding for
// duration of whole program.
void download_magnet(const char* magnet, const char* save_path) {
    try {
        lt::session session = setup_client();

        // Get metadata from magnet
        lt::add_torrent_params atp = lt::parse_magnet_uri(magnet);
        atp.save_path = save_path;

        // Initialise torrent
        lt::torrent_handle t_handle = session.add_torrent(atp);
        //t_handle.set_flags(lt::torrent_flags::sequential_download);

        // Wait for metadata to be retrieved
        std::cout << "Awaiting torrent metadata..." << std::endl;
        while (!t_handle.status().has_metadata) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        // Blocking function
        print_status(t_handle);

        // Done downloading, exit libtorrent
        session.abort();
    }
    catch(const std::exception &e) {
        std::cout << " std::exception thrown: " << e.what();
    }
}

