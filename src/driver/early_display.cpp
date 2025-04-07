#include <driver/early_display.hpp>
#include <io/ports.hpp>

volatile EarlyDisplay::VGAEntry* const EarlyDisplay::framebuffer = reinterpret_cast<volatile EarlyDisplay::VGAEntry*>(0xB8000);
EarlyDisplay::Cursor EarlyDisplay::cursor = {0, 0};
uint8_t EarlyDisplay::attr = 0x0F;

void EarlyDisplay::set_fg(uint8_t color) {
    attr &= 0xF0;
    attr |= color;
}

void EarlyDisplay::set_bg(uint8_t color) {
    attr &= 0x0F;
    attr |= (color << 4);
}

void EarlyDisplay::clear() {
    for (uint8_t y = 0; y < height; y++)
        for (uint8_t x = 0; x < width; x++)
            fb_put(x, y, ' ');
}

void EarlyDisplay::putc(char c) {
    switch (c) {
        case '\n':
            cursor.y++;
            cursor.x = 0;
            break;

        case '\t':
            cursor.x += 4;
            break;

        case '\r':
            cursor.x = 0;
            break;

        case '\b':
            if (cursor.x > 0) {
                cursor.x--;
                fb_put(cursor.x, cursor.y, ' ');
            }
            break;

        default:
            if (c >= ' ') {
                fb_put(cursor.x, cursor.y, c);
                cursor.x++;
            }
    }

    if (cursor.x >= width) {
        cursor.x = 0;
        cursor.y++;
    }

    move_cursor();
    scroll();
}

void EarlyDisplay::scroll() {
    if (cursor.y >= height) {
        memcpy((uint8_t*)framebuffer + width, (uint8_t*)framebuffer, (height - 1) * width * sizeof(VGAEntry));
        memset((uint8_t*)framebuffer + (height - 1) * width, 0, width * sizeof(VGAEntry));
        cursor.y = height - 1;
    }
}

void EarlyDisplay::move_cursor() {
    uint16_t pos = cursor.y * width + cursor.x;
    ports::outb(0x3D4, 0x0F);
    ports::outb(0x3D5, (uint8_t)(pos & 0xFF));
    ports::outb(0x3D4, 0x0E);
    ports::outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void EarlyDisplay::fb_put(uint8_t x, uint8_t y, char c) {
    uint16_t index = y * width + x;
    framebuffer[index].symbol = c;
    framebuffer[index].attr = attr;
}