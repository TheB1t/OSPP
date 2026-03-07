#pragma once

#include <icxxabi.hpp>
#include <klibcpp/utility.hpp>
#include <klibcpp/cstdint.hpp>
#include <driver/serial.hpp>

#define assert(x) kstd::_assert(x, #x, __FILE__, __LINE__)

namespace kstd {
    struct stack_frame {
        stack_frame* ebp;
        uint32_t     eip;
    };

    void _panic(const char* msg);
    void stack_trace(stack_frame* frame, uint32_t max_frames);
    void init_fpu();

    namespace ByteConverter {
        enum Unit {
            B, KB, MB, GB, TB
        };

        constexpr const char* unitNames[] = { "B", "KB", "MB", "GB", "TB", "?" };

        constexpr float unitFactor(Unit unit) {
            switch (unit) {
                case Unit::B:  return 1.0;
                case Unit::KB: return 1024.0;
                case Unit::MB: return 1024.0 * 1024;
                case Unit::GB: return 1024.0 * 1024 * 1024;
                case Unit::TB: return 1024.0 * 1024 * 1024 * 1024;
            }
            return 1.0; // fallback
        }

        float convert(float value, Unit from, Unit to);
        Unit  bestUnit(float value);
    }

    inline void atexit(void (*func)()) {
        auto wrapper = [](void* func_ptr) {
                reinterpret_cast<void (*)()>(func_ptr)();
            };

        __cxa_atexit(wrapper, reinterpret_cast<void*>(func), nullptr);
    }
};