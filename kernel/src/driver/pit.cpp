#include <driver/pit.hpp>
#include <io/ports.hpp>
#include <int/idt.hpp>
#include <log.hpp>

uint64_t          pit::tick_count;
uint32_t          pit::interval_us;

pit::TimerHandler pit::handlers[pit::MAX_HANDLERS] = {};
size_t            pit::handler_count;

bool pit::register_handler(TimerCallback callback, void* arg, TimerTrigger trigger, uint64_t interval_us) {
    if (handler_count >= MAX_HANDLERS || !callback)
        return false;

    for (auto& handler : handlers) {
        if (!handler.active) {
            handler.callback          = callback;
            handler.arg               = arg;
            handler.trigger           = trigger;
            handler.interval_us       = (trigger == TimerTrigger::EveryTick) ? pit::interval_us : interval_us;
            handler.last_triggered_us = tick_count * pit::interval_us;
            handler.active            = true;
            ++handler_count;
            return true;
        }
    }

    return false;
}

bool pit::unregister_handler(TimerCallback callback, void* arg) {
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

void pit::tick_handler(bool has_ext, idt::BaseInterruptContext* base_ctx) {
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
                    should_trigger            = true;
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

void pit::init(uint32_t _interval_us) {
    interval_us   = _interval_us;
    handler_count = 0;
    tick_count    = 0;

    uint16_t divisor = calculate_pit_divisor_us(interval_us);

    ports::outb(PIT_COMMAND, 0x36);  // Channel 0, Access mode: lobyte/hibyte, Mode 3 (Square Wave), Binary

    ports::outb(PIT_CHANNEL_0, divisor & 0xFF);        // Low byte
    ports::outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF); // High byte

    idt::register_irq(0, tick_handler);
}

uint64_t pit::ticks() {
    return tick_count;
}

uint32_t pit::interval() {
    return interval_us;
}

void pit::sleep_ticks(uint32_t target_ticks) {
    uint32_t start = tick_count;
    while ((tick_count - start) < target_ticks)
        __wait_hw_int;
}

void pit::sleep_us(uint32_t target_us) {
    uint32_t target_ticks = target_us / interval_us;
    target_ticks = target_ticks == 0 ? 1 : target_ticks;
    sleep_ticks(target_ticks);
}