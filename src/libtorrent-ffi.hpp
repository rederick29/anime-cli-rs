#ifndef LIBTORRENT_FFI_H
#define LIBTORRENT_FFI_H
#endif
#include "libtorrent/session.hpp"
#include "libtorrent/add_torrent_params.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/torrent_flags.hpp"

// Download given magnet link, returning path to first file downloaded
extern "C" void download_magnet(const char* magnet, const char* save_path);

lt::session setup_client();
void print_status(lt::torrent_handle handle);

