#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/array.hpp>
#include <int/idt.hpp>

class pit {
    public:
        using TimerCallback = void (*)(idt::InterruptFrame*, void*);

        enum class TimerTrigger {
            EveryTick,
            Interval,
            OneShot
        };

        struct TimerHandler {
            TimerCallback callback;
            void*         arg;
            TimerTrigger  trigger;
            uint64_t      interval_us;
            uint64_t      last_triggered_us;
            bool          active;
        };

        static constexpr uint16_t PIT_CHANNEL_0      = 0x40;
        static constexpr uint16_t PIT_COMMAND        = 0x43;
        static constexpr uint32_t PIT_BASE_FREQUENCY = 1193182;
        static constexpr uint16_t calculate_pit_divisor_us(uint32_t target_microseconds) {
            return static_cast<uint16_t>(
                (PIT_BASE_FREQUENCY * target_microseconds + 500'000) / 1'000'000
            );
        }

        static constexpr size_t MAX_HANDLERS = 8;

        static bool     register_handler(TimerCallback callback, void* arg, TimerTrigger trigger,
            uint64_t interval_ticks);
        static bool     unregister_handler(TimerCallback callback, void* arg);

        static void     init(uint32_t interval_us);
        static uint64_t ticks();
        static uint32_t interval();

        static void     sleep_ticks(uint32_t target_ticks);
        static void     sleep_us(uint32_t target_us);

    private:
        static void     tick_handler(idt::BaseInterruptFrame* base_ctx);

        static uint64_t tick_count;
        static uint32_t interval_us;
        static TimerHandler handlers[MAX_HANDLERS];
        static size_t handler_count;
};