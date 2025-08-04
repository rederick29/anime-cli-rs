#include "ffi-data.hpp"
#include <cstring>

namespace lt_ffi {
file_list* create_file_list(const std::vector<std::tuple<int64_t, int64_t, size_t, std::string>> in) {
    file_list* list = new file_list;
    if (in.empty()) {
        list->count = 0;
        list->files[0]->size = 0;
        list->files[0]->offset = 0;
        list->files[0]->priority = 0;
        list->files[0]->path = strdup("empty");
    }

    list->count = in.size();
    list->files = new file*[list->count];

    for (size_t i = 0; i < list->count; i++) {
        list->files[i] = new file;
        const auto& tuple = in[i];
        file* f = list->files[i];

        f->size = std::get<0>(tuple);
        f->offset = std::get<1>(tuple);
        f->priority = std::get<2>(tuple);

        f->path = new char[std::get<3>(tuple).size() + 1];
        strncpy(f->path, std::get<3>(tuple).c_str(), std::get<3>(tuple).size() + 1);
    }

    return list;
}

void free_file_list(file_list* list) {
    if (!list) return;

    for (size_t i = 0; i < list->count; i++) {
        delete[] list->files[i]->path;
        delete list->files[i];
    }
    delete[] list->files;
    delete list;
}
} // namespace lt_ffi
