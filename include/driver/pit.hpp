#pragma once

#include <klibcpp/cstdint.hpp>

namespace pit {
    constexpr uint16_t PIT_CHANNEL_0 = 0x40;
    constexpr uint16_t PIT_COMMAND   = 0x43;
    constexpr uint32_t PIT_BASE_FREQUENCY = 1193182;
    constexpr uint16_t calculate_pit_divisor_us(uint32_t target_microseconds) {
        return static_cast<uint16_t>(
            (PIT_BASE_FREQUENCY * target_microseconds + 500'000) / 1'000'000
        );
    }

    void init(uint32_t interval_us);
    uint64_t ticks();
    uint32_t interval();
}