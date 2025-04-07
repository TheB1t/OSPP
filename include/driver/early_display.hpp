#pragma once

#include <klibc/cstdint.hpp>
#include <klibc/cstring.hpp>
#include <printf_impl.hpp>

class EarlyDisplay {
    public:
        struct VGAEntry {
            uint8_t    symbol;
            uint8_t    attr;
        };

        struct Cursor {
            uint8_t x;
            uint8_t y;
        };

        static constexpr uint8_t width = 80;
        static constexpr uint8_t height = 25;

        static void set_fg(uint8_t color);
        static void set_bg(uint8_t color);
        static void clear();

        template<typename ...Args>
        static void printf(const char* format, Args... args) {
            _printf(EarlyDisplay::putc, format, args...);
        }

        static void putc(char c);

    private:
        static volatile VGAEntry* const framebuffer;
        static Cursor cursor;
        static uint8_t attr;

        static void scroll();
        static void move_cursor();
        static void fb_put(uint8_t x, uint8_t y, char c);
};
