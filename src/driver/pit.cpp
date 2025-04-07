#include <driver/pit.hpp>
#include <io/ports.hpp>
#include <int/idt.hpp>

namespace pit {
    static uint64_t tick_count = 0;

    void init(uint16_t divisor) {
        ports::outb(PIT_COMMAND, 0x36);  // Channel 0, Access mode: lobyte/hibyte, Mode 3 (Square Wave), Binary

        ports::outb(PIT_CHANNEL_0, divisor & 0xFF);        // Low byte
        ports::outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF); // High byte

        idt::register_irq(0, tick_handler);
    }

    void tick_handler(bool, idt::BaseInterruptContext*) {
        // serial::printf("Tick!\n");
        ++tick_count;
    }

    uint64_t ticks() {
        return tick_count;
    }

    void wait(uint64_t target_ticks) {
        uint64_t start = tick_count;
        while ((tick_count - start) < target_ticks) {
            asm volatile("hlt");
        }
    }
}
