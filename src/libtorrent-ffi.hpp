#pragma once
#include "ffi-data.hpp"
#include "BittorrentClient.hpp"

extern "C" BittorrentClient* create_client();
extern "C" void set_client_options(BittorrentClient* client, const lt_ffi::lt_settings* opts);
extern "C" void print_status(BittorrentClient* client);
extern "C" bool is_finished(BittorrentClient* client);
extern "C" lt_ffi::file_list* add_torrent(BittorrentClient* client, const char* magnet, const char* save_path);

extern "C" void free_file_list(lt_ffi::file_list* files);
