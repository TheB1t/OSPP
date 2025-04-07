#pragma once

#include <klibcpp/cstdint.hpp>
#include <int/idt.hpp>

namespace pit {
    constexpr uint16_t PIT_CHANNEL_0 = 0x40;
    constexpr uint16_t PIT_COMMAND   = 0x43;

    enum class Mode : uint8_t {
        InterruptOnTerminalCount = 0,
        OneShot = 1,
        RateGenerator = 2,
        SquareWave = 3,
        SoftwareStrobe = 4,
        HardwareStrobe = 5
    };

    constexpr uint32_t PIT_BASE_FREQUENCY = 1193182;
    constexpr uint16_t calculate_pit_divisor_us(uint32_t target_microseconds) {
        return static_cast<uint16_t>(
            (PIT_BASE_FREQUENCY * target_microseconds + 500'000) / 1'000'000
        );
    }

    void init(uint16_t frequency);
    void tick_handler(bool, idt::BaseInterruptContext*);
    uint64_t ticks();
    void wait(uint64_t target_ticks);
}