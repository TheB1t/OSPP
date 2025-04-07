#pragma once

#include <klibcpp/cstdint.hpp>

namespace kstd {
    struct true_type {
        static constexpr bool value = true;
    };

    struct false_type {
        static constexpr bool value = false;
    };

    template <typename T, typename U>
    struct is_same : false_type{};

    template <typename T>
    struct is_same<T, T> : true_type{};

    template <typename T>
    struct is_int : false_type{};
    template <>
    struct is_int<int> : true_type{};
    template <>
    struct is_int<size_t> : true_type{};

    template <typename T>
    struct is_32bit_int : false_type{};
    template <>
    struct is_32bit_int<int> : true_type{};

    template <typename T>
    struct is_64bit_int : false_type{};
    template <>
    struct is_64bit_int<size_t> : true_type{};
    template <>
    struct is_64bit_int<int64_t> : true_type{};

    template <typename T>
    struct is_void : false_type{};
    template <>
    struct is_void<void> : true_type{};

    template <typename T>
    struct is_pointer : false_type{};
    template <typename T>
    struct is_pointer<T *> : true_type{};

    template <typename T>
    struct is_reference : false_type{};
    template <typename T>
    struct is_reference<T &> : true_type{};

    template <typename T>
    struct is_rvalue_reference : false_type{};
    template <typename T>
    struct is_rvalue_reference<T &&> : true_type{};


    template <typename T, typename U>
    constexpr bool is_same_v = is_same<T, U>::value;

    template <typename T>
    constexpr bool is_int_v = is_int<T>::value;

    template <typename T>
    constexpr bool is_32bit_int_v = is_32bit_int<T>::value;

    template <typename T>
    constexpr bool is_64bit_int_v = is_64bit_int<T>::value;

    template <typename T>
    constexpr bool is_void_v = is_void<T>::value;

    template <typename T>
    constexpr bool is_pointer_v = is_pointer<T>::value;

    template <typename T>
    constexpr bool is_reference_v = is_reference<T>::value;

    template <typename T>
    constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;


    template<typename T>
    struct is_enum {
    private:
        template<typename U>
        static constexpr bool test(int) {
            return __is_enum(U);
        }

        template<typename>
        static constexpr bool test(...) {
            return false;
        }

    public:
        static constexpr bool value = test<T>(0);
    };

    template<typename T>
    inline constexpr bool is_enum_v = is_enum<T>::value;



    template <size_t... Is>
    struct index_sequence {};

    template <size_t N, size_t... Is>
    struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};

    template <size_t... Is>
    struct make_index_sequence_impl<0, Is...> {
        using type = index_sequence<Is...>;
    };

    template <size_t N>
    using make_index_sequence = typename make_index_sequence_impl<N>::type;
}