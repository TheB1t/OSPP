#pragma once

#include <klibcpp/type_traits.hpp>

namespace kstd {
    template<typename T>
    constexpr typename remove_reference<T>::type&& move(T&& t) noexcept {
        return static_cast<typename remove_reference<T>::type&&>(t);
    }
}