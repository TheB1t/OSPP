#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/hash.hpp>

namespace kstd {
    template<typename T>
    constexpr uint64_t type_id() {
    #if defined(__clang__) || defined(__GNUC__)
        return hash_fold32_cstr(__PRETTY_FUNCTION__);
    #elif defined(_MSC_VER)
        return hash_fold32_cstr(__FUNCSIG__);
    #else
        return 0;
    #endif
    }

    template<typename T>
    constexpr uint64_t type_id(__unused T& __garbage) {
        return type_id<T>();
    }
}