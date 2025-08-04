#include "BittorrentClient.hpp"
#include <iostream>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <tuple>

BittorrentClient::BittorrentClient() {
    settings = lt::default_settings();

    settings.set_str(lt::settings_pack::dht_bootstrap_nodes, "router.utorrent.com:6881,dht.transmissionbt.com:6881,dht.libtorrent.org:25401");
    settings.set_int(lt::settings_pack::alert_mask,
        lt::alert_category::storage
        | lt::alert_category::stats
        | lt::alert_category::error);

    // prefer tcp for streaming
    settings.set_int(lt::settings_pack::mixed_mode_algorithm, lt::settings_pack::prefer_tcp);
    session = std::make_unique<lt::session>(settings);
}

void BittorrentClient::set_options(const lt_ffi::lt_settings* opts) {
    settings.set_int(lt::settings_pack::connections_limit, opts->connection_limit);
    settings.set_bool(lt::settings_pack::enable_outgoing_utp, opts->enable_utp);
    settings.set_bool(lt::settings_pack::enable_incoming_utp, opts->enable_utp);
    lt::settings_pack::enc_policy encryption = opts->force_encryption ? lt::settings_pack::enc_policy::pe_forced : opts->enable_encryption ? lt::settings_pack::enc_policy::pe_enabled : lt::settings_pack::enc_policy::pe_disabled;
    settings.set_int(lt::settings_pack::out_enc_policy, encryption);
    settings.set_int(lt::settings_pack::in_enc_policy, encryption);
    settings.set_str(lt::settings_pack::listen_interfaces, std::string(opts->listen_interfaces));
    session->apply_settings(settings);
}

// string ends_with because somehow this is only in C++20
inline bool ends_with(std::string const& haystack, std::string const& needle) {
    return needle.length() < haystack.length() && std::equal(needle.rbegin(), needle.rend(), haystack.rbegin());
}

lt_ffi::file_list* BittorrentClient::add_torrent(const char* magnet, const char* save_path) {
    // the client handles a single torrent at a time
    if (handle.is_valid()) {
        return nullptr;
    }

    lt::add_torrent_params atp = lt::parse_magnet_uri(magnet);
    atp.save_path = save_path;

    handle = session->add_torrent(atp);
    std::cout << "Awaiting torrent metadata..." << std::endl;
    while (!handle.status().has_metadata) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::shared_ptr<const lt::torrent_info> t_info = handle.torrent_file();
    lt::file_storage t_files = t_info->files();

    size_t priority = 0;
    std::vector<std::tuple<int64_t, int64_t, size_t, std::string>> files;

    for (auto i = 0; i < t_files.num_files(); i++) {
        // TODO: only deal with mkvs and mp4s for now, do this properly later (never)
        std::string filename = t_files.file_path(i);
        if (ends_with(filename, ".mkv") || ends_with(filename, ".mp4")) {
            files.emplace_back(std::make_tuple(t_files.file_size(i), t_files.file_offset(i), priority++, filename));
        } else {
            handle.file_priority(i, lt::dont_download);
        }
    }

    lt_ffi::file_list* ret = lt_ffi::create_file_list(files);
    if (ret->count == 0) {
        std::cout << "no playable files found, exiting..." << std::endl;
        session->abort();
        return ret;
    }

    return ret;
}

// blocking lt thread
void BittorrentClient::print_status() {
    lt::torrent_status t_status = handle.status();

    std::string states[] = { "Checking Files", "Downloading Metadata", "Downloading", "Finished",
                             "Seeding", "Allocating", "Checking Resume Data" };

    t_status = handle.status();
    std::cout << "\r" << "libtorrent " << lt::version() << " "
        << states[t_status.state-1] << " " << std::round(t_status.progress*100)
        << "% " << t_status.download_rate/1000 << " KB/s down " << t_status.upload_rate/1000
        << " KB/s up Peers:" << t_status.num_peers << "       "; // empty space for ovewriting any characters left
    std::cout << std::flush;
}

// blocking lt thread
bool BittorrentClient::is_finished() {
    return handle.status().is_finished;
}

