#pragma once

#include <fstream>
#include <iostream>
#include <string>

inline bool DecodeRLE(const std::string& src_name, const std::string& dst_name) {
    using namespace std;

    ifstream in(src_name, ios::binary);
    if (!in) {
        return false;
    }

    ofstream out(dst_name, ios::binary);

    do {
        unsigned char header = static_cast<unsigned char>(in.get());
        int block_type = (header & 1);
        int data_size = (header >> 1) + 1;

        char data;
        if (block_type == 0) {
            for (int i = 0; i < data_size; i++) {
                data = in.get();
                if (!in.eof()) {
                    out << data;
                }
            }
        } else {
            data = in.get();
            if (!in.eof()) {
                std::string extract(data_size, data);
                out << extract;
            }
        }
    } while (in);

    return true;
}