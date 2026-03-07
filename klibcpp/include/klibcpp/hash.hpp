#pragma once

#include <klibcpp/cstdint.hpp>

namespace kstd {
    constexpr uint32_t hash_fold32_cstr(const char* s) {
        uint64_t h = 1469598103934665603ULL;
        while (*s) {
            h ^= (uint8_t)*s++;
            h *= 1099511628211ULL;
        }
        return (uint32_t)(h ^ (h >> 32));
    }

    template<typename T>
    constexpr uint32_t hash_fold32_trivial(const T& x) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
        uint64_t       h = 1469598103934665603ULL;
        for (size_t i = 0; i < sizeof(T); ++i) {
            h ^= p[i];
            h *= 1099511628211ULL;
        }
        return (uint32_t)(h ^ (h >> 32));
    }
}