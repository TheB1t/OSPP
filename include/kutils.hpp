#pragma once

#include <klibc/cstdint.hpp>
#include <driver/serial.hpp>

#define assert(x) kutils::_assert(x, #x, __FILE__, __LINE__)

namespace kutils {
    template<typename ...Args>
    static inline void panic(const char* fmt, Args... args) {
        serial::printf("Kernel panic:\n    ");
        serial::printf(fmt, args...);
        serial::printf("\n");

        for (;;);
        __builtin_unreachable();
    }

    static inline void _assert(bool cond, const char* cond_s, const char* file, int line) {
        if (!cond) {
            serial::printf("Assertion failed: %s\n    File: %s\n    Line: %d\n", cond_s, file, line);
            __builtin_unreachable();
        }
    }

    namespace ByteConverter {
        enum Unit {
            B, KB, MB, GB, TB
        };

        constexpr const char* unitNames[] = { "B", "KB", "MB", "GB", "TB", "?" };

        constexpr double unitFactor(Unit unit) {
            switch (unit) {
                case Unit::B:  return 1.0;
                case Unit::KB: return 1024.0;
                case Unit::MB: return 1024.0 * 1024;
                case Unit::GB: return 1024.0 * 1024 * 1024;
                case Unit::TB: return 1024.0 * 1024 * 1024 * 1024;
            }
            return 1.0; // fallback
        }

        inline double convert(double value, Unit from, Unit to) {
            double bytes = value * unitFactor(from);
            return bytes / unitFactor(to);
        }

        inline Unit bestUnit(double value) {
            Unit units[] = { Unit::B, Unit::KB, Unit::MB, Unit::GB, Unit::TB };
            int i = 0;
            while (i < 4 && value >= 1024.0) {
                value /= 1024.0;
                ++i;
            }
            return units[i];
        }
    }

};