#pragma once

#include <icxxabi.hpp>
#include <klibcpp/cstdint.hpp>
#include <driver/serial.hpp>

#define assert(x) kstd::_assert(x, #x, __FILE__, __LINE__)

namespace kstd {
    template<typename T>
    struct remove_reference {
        typedef T type;
    };

    template<typename T>
    struct remove_reference<T&> {
        typedef T type;
    };

    template<typename T>
    struct remove_reference<T&&> {
        typedef T type;
    };

    template<typename T>
    constexpr T&& forward(typename remove_reference<T>::type& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T>
    constexpr T&& forward(typename remove_reference<T>::type&& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T>
    constexpr typename remove_reference<T>::type&& move(T&& t) noexcept {
        return static_cast<typename remove_reference<T>::type&&>(t);
    }

    struct stack_frame {
        stack_frame* ebp;
        uint32_t eip;
    };

    void stack_trace(stack_frame* frame, uint32_t max_frames);

    template<typename ...Args>
    inline void panic(const char* fmt, Args&&... args) {
        serial::printf("\n\nKernel panic:\n    ");
        serial::printf(fmt, kstd::forward<Args>(args)...);
        serial::printf("\n");


        stack_frame* stk;
        __asm__ volatile("movl %%ebp, %0" : "=r"(stk));

        stack_trace(stk, 10);
        __unreachable();
    }

    inline void _assert(bool cond, const char* cond_s, const char* file, int line) {
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

    inline void atexit(void (*func)()) {
        auto wrapper = [](void* func_ptr) {
            reinterpret_cast<void (*)()>(func_ptr)();
        };

        __cxa_atexit(wrapper, reinterpret_cast<void*>(func), nullptr);
    }

    template<int N>
    inline void trigger_interrupt() {
        asm volatile ("int %0" : : "i"(N) : "memory");
    }
};