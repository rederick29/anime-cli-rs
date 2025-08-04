#pragma once
#include "ffi-data.hpp"
#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <memory>

// Single-torrent client
class BittorrentClient {
public:
    BittorrentClient();
    ~BittorrentClient();
    void set_options(const lt_ffi::lt_settings* opts);
    void print_status();
    bool is_finished();
    lt_ffi::file_list* add_torrent(const char* magnet, const char* save_path);
private:
    lt::settings_pack settings;
    std::unique_ptr<lt::session> session;
    lt::torrent_handle handle;
};
