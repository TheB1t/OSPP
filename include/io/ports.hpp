#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/type_traits.hpp>
#include <register.hpp>

class ports {
    public:
        template<typename T>
        static inline T in(uint16_t port) {
            static_assert(
                kstd::is_same_v<T, uint8_t> ||
                kstd::is_same_v<T, uint16_t> ||
                kstd::is_same_v<T, uint32_t>,
                "Unsupported port type. Use uint8_t, uint16_t, or uint32_t"
            );

            T ret;

            if constexpr (kstd::is_same_v<T, uint8_t>)
                INLINE_ASSEMBLY("inb %%dx, %%al" : "=a"(ret) : "d"(port));
            else if constexpr (kstd::is_same_v<T, uint16_t>)
                INLINE_ASSEMBLY("inw %%dx, %%ax" : "=a"(ret) : "d"(port));
            else if constexpr (kstd::is_same_v<T, uint32_t>)
                INLINE_ASSEMBLY("inl %%dx, %%eax" : "=a"(ret) : "d"(port));

            return ret;
        }

        template<typename T>
        static inline void out(uint16_t port, T data) {
            static_assert(
                kstd::is_same_v<T, uint8_t> ||
                kstd::is_same_v<T, uint16_t> ||
                kstd::is_same_v<T, uint32_t>,
                "Unsupported port type. Use uint8_t, uint16_t, or uint32_t"
            );

            if constexpr (kstd::is_same_v<T, uint8_t>)
                INLINE_ASSEMBLY("outb %%al, %%dx" : : "a"(data), "d"(port));
            else if constexpr (kstd::is_same_v<T, uint16_t>)
                INLINE_ASSEMBLY("outw %%ax, %%dx" : : "a"(data), "d"(port));
            else if constexpr (kstd::is_same_v<T, uint32_t>)
                INLINE_ASSEMBLY("outl %%eax, %%dx" : : "a"(data), "d"(port));
        }

        // Explicit size variants
        static inline uint8_t inb(uint16_t port) {
            return in<uint8_t>(port);
        }

        static inline uint16_t inw(uint16_t port) {
            return in<uint16_t>(port);
        }

        static inline uint32_t inl(uint16_t port) {
            return in<uint32_t>(port);
        }

        static inline void outb(uint16_t port, uint8_t data) {
            out<uint8_t>(port, data);
        }

        static inline void outw(uint16_t port, uint16_t data) {
            out<uint16_t>(port, data);
        }

        static inline void outl(uint16_t port, uint32_t data) {
            out<uint32_t>(port, data);
        }

        template<typename T>
        struct Port : public RegisterBase<Port<T>, T> {
            uint16_t address;

            using RegisterBase<Port<T>, T>::operator=;

            Port(uint16_t address) : address(address) {}

            T read() const volatile {
                return ports::in<T>(address);
            }

            void write(T value) const volatile {
                ports::out<T>(address, value);
            }
        };

        using Port8 = Port<uint8_t>;
        using Port16 = Port<uint16_t>;
        using Port32 = Port<uint32_t>;
};