#include <driver/pit.hpp>
#include <io/ports.hpp>
#include <int/idt.hpp>
#include <log.hpp>

namespace pit {
    uint64_t tick_count = 0;
    uint32_t interval_us = 0;

    struct TimerHandler {
        TimerCallback callback = nullptr;
        void* arg = nullptr;
        TimerTrigger trigger = TimerTrigger::EveryTick;
        uint64_t interval_us = 1000; // Default to 1ms
        uint64_t last_triggered_us = 0;
        bool active = false;
    };

    kstd::array<TimerHandler, MAX_HANDLERS> handlers{};
    size_t handler_count = 0;

    bool register_handler(TimerCallback callback, void* arg, TimerTrigger trigger, uint64_t interval_us) {
        if (handler_count >= MAX_HANDLERS || !callback)
            return false;

        for (auto& handler : handlers) {
            if (!handler.active) {
                handler.callback = callback;
                handler.arg = arg;
                handler.trigger = trigger;
                handler.interval_us = (trigger == TimerTrigger::EveryTick) ? pit::interval_us : interval_us;
                handler.last_triggered_us = tick_count * pit::interval_us;
                handler.active = true;
                ++handler_count;
                return true;
            }
        }

        return false;
    }

    bool unregister_handler(TimerCallback callback, void* arg) {
        if (!callback)
            return false;

        for (auto& handler : handlers) {
            if (handler.active && handler.callback == callback && handler.arg == arg) {
                handler.active = false;
                --handler_count;
                return true;
            }
        }

        return false;
    }

    void tick_handler(bool has_ext, idt::BaseInterruptContext* base_ctx) {
        ++tick_count;
        uint64_t current_time_us = tick_count * interval_us;

        if (!has_ext) {
            LOG_WARN("There's no extended interrupt context for the PIT, TimerTriggered handlers won't work!\n");
            return;
        }

        for (auto& handler : handlers) {
            if (!handler.active) continue;

            bool should_trigger = false;

            switch (handler.trigger) {
                case TimerTrigger::EveryTick:
                    should_trigger = true;
                    break;
                case TimerTrigger::Interval:
                    if (current_time_us - handler.last_triggered_us >= handler.interval_us) {
                        should_trigger = true;
                        handler.last_triggered_us = current_time_us;
                    }
                    break;
                case TimerTrigger::OneShot:
                    if (current_time_us - handler.last_triggered_us >= handler.interval_us) {
                        should_trigger = true;
                        handler.active = false; // Deactivate after one shot
                        --handler_count;
                    }
                    break;
            }

            if (should_trigger && handler.callback) {
                idt::InterruptContext* ctx = idt::InterruptContext::from_base(base_ctx);
                handler.callback(ctx, handler.arg);
            }
        }
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