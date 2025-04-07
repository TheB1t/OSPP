#pragma once

#include <klibcpp/cstdint.hpp>
#include <io/ports.hpp>

namespace pic {
    constexpr uint8_t PIC1_COMMAND = 0x20;
    constexpr uint8_t PIC1_DATA    = 0x21;
    constexpr uint8_t PIC2_COMMAND = 0xA0;
    constexpr uint8_t PIC2_DATA    = 0xA1;

    constexpr uint8_t ICW1_INIT    = 0x10;
    constexpr uint8_t ICW1_ICW4    = 0x01;
    constexpr uint8_t ICW4_8086    = 0x01;

    inline void io_wait() {
        ports::outb(0x80, 0);
    }

    inline void remap(uint8_t offset1 = 0x20, uint8_t offset2 = 0x28) {
        uint8_t a1 = ports::inb(PIC1_DATA);
        uint8_t a2 = ports::inb(PIC2_DATA);

        ports::outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
        ports::outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();

        ports::outb(PIC1_DATA, offset1); io_wait();
        ports::outb(PIC2_DATA, offset2); io_wait();

        ports::outb(PIC1_DATA, 0x04); io_wait();
        ports::outb(PIC2_DATA, 0x02); io_wait();

        ports::outb(PIC1_DATA, ICW4_8086); io_wait();
        ports::outb(PIC2_DATA, ICW4_8086); io_wait();

        ports::outb(PIC1_DATA, a1);
        ports::outb(PIC2_DATA, a2);
    }

    inline void EOI(uint8_t irq) {
        if (irq >= 8)
            ports::outb(PIC2_COMMAND, 0x20);

        ports::outb(PIC1_COMMAND, 0x20);
    }

    inline void disable() {
        ports::outb(PIC1_DATA, 0xFF);
        ports::outb(PIC2_DATA, 0xFF);
    }
}