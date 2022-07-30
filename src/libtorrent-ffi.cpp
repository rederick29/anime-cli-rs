#include "libtorrent-ffi.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

// Download provided magnet link.
// TODO: This currently leeches, should rather
// be moved to different thread and kept seeding for
// duration of whole program.
char* download_magnet(const char* magnet) {
    try {
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

        // Get metadata from magnet
        lt::add_torrent_params atp = lt::parse_magnet_uri(magnet);
        // Set temporary save path
        atp.save_path = "/tmp/";
        // Initialise torrent
        lt::torrent_handle t_handle = session.add_torrent(atp);
        // Make sequential for video playing WIP
        // t_handle.set_flags(lt::torrent_flags::sequential_download);

        // Wait for metadata to be retrieved
        std::cout << "Awaiting torrent metadata..." << std::endl;
        while (!t_handle.status().has_metadata) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        // Save status, ptr to info and files.
        lt::torrent_status t_status = t_handle.status();
        std::shared_ptr<const lt::torrent_info> t_info = t_handle.torrent_file();
        lt::file_storage t_files = t_info->files();

        // Check if there is at least 1 file in torrent
        if (t_files.num_files() < 1) {
            throw std::runtime_error("torrent has no files!");
        }

        // Print the file(s) save path
        for (int i = 0; i < t_files.num_files(); i++) {
            std::string file_path = t_status.save_path + t_files.file_path(i);
            std::cout << "Saving file " << i+1 << " to " << file_path << std::endl;
        }

        // Save path of first file - TODO: Don't just pick the first file
        std::string file_path = t_status.save_path + t_files.file_path(0);

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
        // Done downloading, exit libtorrent
        session.abort();

        // Make new file path string to return to rust
        char* output_path = (char*) malloc(file_path.length()+1);
        std::strncpy(output_path, file_path.c_str(), file_path.length()+1);
        if (output_path == nullptr) {
            throw std::runtime_error("c++ output_path is nullptr");
        } else {
            return output_path;
        }
    }
    catch(const std::exception &e) {
        std::cout << " std::exception thrown: " << e.what();
    }
    return nullptr;
}
