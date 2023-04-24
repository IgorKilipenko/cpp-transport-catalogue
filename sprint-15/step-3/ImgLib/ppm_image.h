#pragma once
#include <filesystem>

#include "img_lib.h"

namespace img_lib {
    using Path = std::filesystem::path;

    bool SavePPM(const Path& file, const Image& image);
    Image LoadPPM(const Path& file);

}  // namespace img_lib