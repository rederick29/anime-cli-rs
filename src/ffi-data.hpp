#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <tuple>

namespace lt_ffi {
typedef struct lt_settings {
    uint16_t connection_limit;
    bool enable_utp;
    bool enable_encryption;
    bool force_encryption;
    const char* listen_interfaces;
} lt_settings;

// TODO: use the priorities to change order of files being played by the player
typedef struct file {
    int64_t size;
    int64_t offset;
    size_t priority;
    char* path;
} file;

typedef struct file_list {
    size_t count;
    file** files;
} file_list;

file_list* create_file_list(const std::vector<std::tuple<int64_t, int64_t, size_t, std::string>> files);
void free_file_list(file_list* files);
} // namespace lt_ffi
