#include "libtorrent-ffi.hpp"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <iostream>

// Download provided magnet link.
// TODO: This currently leeches, should rather
// be moved to different thread and kept seeding for
// duration of whole program.
char* download_magnet(const char* magnet) {
    try {
        // Start new libtorrent session
        lt::session session;
        // Get metadata from magnet
        lt::add_torrent_params atp = lt::parse_magnet_uri(magnet);
        // Set temporary save path
        atp.save_path = "/tmp/";
        // Initialise torrent
        lt::torrent_handle t_handle = session.add_torrent(atp);

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

        // Until it finishes downloading, poll status and write out percentage every 2s
        bool finished = t_status.is_finished;
        while (!finished) {
            t_status = t_handle.status();
            std::cout << "\r";
            printf("%.1f%% complete", t_status.progress*100);
            std::cout << std::flush;
            finished = t_status.is_finished;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        // Done downloading, exit libtorrent
        session.abort();

        // Make new file path string to return to rust
        char* output_path = (char*) malloc(file_path.length()+1);
        std::strncpy(output_path, file_path.c_str(), file_path.length()+1);
        std::cout << "[src/libtorrent-ffi.cpp:62] output_path = " << output_path << std::endl; //debug 
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
