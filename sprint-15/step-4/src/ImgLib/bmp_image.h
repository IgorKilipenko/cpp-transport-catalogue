#pragma once
#include <filesystem>

#include "img_lib.h"
#include "pack_defines.h"

namespace img_lib {
    using Path = std::filesystem::path;

    PACKED_STRUCT_BEGIN BitmapFileHeader {
        // поля заголовка Bitmap File Header
        char signature[2] = {'B', 'M'};
        uint32_t file_size;
        uint32_t reserved_space = 0x000000;
        uint32_t file_offset = 54;
    }
    PACKED_STRUCT_END

    PACKED_STRUCT_BEGIN BitmapInfoHeader {
        // поля заголовка Bitmap Info Header
        uint32_t info_header_size = 40;
        int32_t image_width = 0;
        int32_t image_height = 0;
        uint16_t planes_count = 1;
        uint16_t bit_per_pixel = 24;
        uint32_t compression = 0;
        uint32_t image_size = 0;
        int32_t x_ppm = 11811;
        int32_t y_ppm = 11811;
        int32_t colors_in_color_table = 0;
        int32_t important_color_count = 0x1000000;
    }
    PACKED_STRUCT_END
    bool SaveBMP(const Path& file, const Image& image);
    Image LoadBMP(const Path& file);

}  // of namespace img_lib