#pragma once
#include <filesystem>

#include "img_lib.h"

namespace img_lib {
    using Path = std::filesystem::path;

    bool SaveJPEG(const Path& file, const Image& image);
    Image LoadJPEG(const Path& file);

}  // of namespace img_lib