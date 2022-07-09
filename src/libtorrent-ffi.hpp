#ifndef LIBTORRENT_FFI_H
#define LIBTORRENT_FFI_H
#endif 
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/torrent_status.hpp"

/**
 * @brief Download given magnet link, returning when done.
 * 
 * @param magnet - magnet link to be parsed
 * @return true  - Finished downloading successfully
 * @return false - Error
 */
bool download_magnet(char const* magnet);
