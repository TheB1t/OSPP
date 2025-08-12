#pragma once

#include <klibcpp/cstdint.hpp>

namespace kstd {
    template <typename T, size_t N>
    struct array {
        T data[N];
        
        constexpr T& operator[](size_t i) {
            return data[i]; 
        }

        constexpr const T& operator[](size_t i) const { 
            return data[i];
        }

        constexpr size_t size() const {
            return N;
        }

        constexpr T* begin() {
            return data;
        }

        constexpr const T* begin() const {
            return data;
        }

        constexpr T* end() {
            return data + N;
        }

        constexpr const T* end() const {
            return data + N;
        }
    };
}