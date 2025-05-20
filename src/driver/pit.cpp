#include <driver/pit.hpp>
#include <io/ports.hpp>
#include <int/idt.hpp>

namespace pit {
    static uint64_t tick_count = 0;
    static uint32_t interval_us = 0;

    void tick_handler(bool, idt::BaseInterruptContext*) {
        ++tick_count;
    }

    void init(uint32_t interval_us) {
        pit::interval_us = interval_us;
        uint16_t divisor = calculate_pit_divisor_us(interval_us);
        ports::outb(PIT_COMMAND, 0x36);  // Channel 0, Access mode: lobyte/hibyte, Mode 3 (Square Wave), Binary

        ports::outb(PIT_CHANNEL_0, divisor & 0xFF);        // Low byte
        ports::outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF); // High byte

        idt::register_irq(0, tick_handler);
    }

    uint64_t ticks() {
        return tick_count;
    }

    uint32_t interval() {
        return interval_us;
    }
}
