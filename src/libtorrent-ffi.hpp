#ifndef LIBTORRENT_FFI_H
#define LIBTORRENT_FFI_H
#endif 
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/torrent_status.hpp"

// Download given magnet link, returning path to first file downloaded
char* download_magnet(const char* magnet);
