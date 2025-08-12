#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/kstd.hpp>
#include <klibcpp/printf_impl.hpp>

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

        class EarlyDisplayOutputter : public BaseOutputter {
            virtual void write(char c) override {
                EarlyDisplay::putc(c);
            }
        };

        template<typename ...Args>
        static void printf(const char* format, Args&&... args) {
            static EarlyDisplayOutputter outputter;
            _printf(outputter, format, kstd::forward<Args>(args)...);
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
