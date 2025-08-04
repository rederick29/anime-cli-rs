#include "libtorrent-ffi.hpp"

extern "C" BittorrentClient* create_client() {
    return new BittorrentClient();
}

extern "C" void set_client_options(BittorrentClient* client, const lt_ffi::lt_settings* opts) {
    client->set_options(opts);
}

extern "C" void print_status(BittorrentClient* client) {
    client->print_status();
}

extern "C" bool is_finished(BittorrentClient* client) {
    return client->is_finished();
}

extern "C" lt_ffi::file_list* add_torrent(BittorrentClient* client, const char* magnet, const char* save_path) {
    return client->add_torrent(magnet, save_path);
}

extern "C" void free_file_list(lt_ffi::file_list* files) {
    lt_ffi::free_file_list(files);
}
