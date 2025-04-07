#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/array.hpp>

namespace crc32 {
    constexpr auto generate_crc32_table() {
        kstd::array<uint32_t, 256> table{};

        constexpr uint32_t polynomial = 0xEDB88320;
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (size_t j = 0; j < 8; j++) {
                if (c & 1) {
                    c = polynomial ^ (c >> 1);
                } else {
                    c >>= 1;
                }
            }
            table[i] = c;
        }
        return table;
    }

    static const auto crc32_table = generate_crc32_table();

    static uint32_t calc(const void* buf, size_t len, uint32_t initial = 0) {
        uint32_t c = initial ^ 0xFFFFFFFF;
        const uint8_t* u = static_cast<const uint8_t*>(buf);
        
        for (size_t i = 0; i < len; ++i) {
            c = crc32_table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
        }
        
        return c ^ 0xFFFFFFFF;
    }
};
