#pragma once

#include <klibcpp/cstdint.hpp>

namespace kstd {
    class InterruptGuard {
        public:
            InterruptGuard() : should_enable(false) {
                uint32_t flags;

                INLINE_ASSEMBLY(
                    "pushfl\n"
                    "popl %0\n"
                    "cli\n"
                    : "=r"(flags)
                    :
                    : "memory"
                );

                if (flags & (1 << 9))
                    should_enable = true;
            }

            ~InterruptGuard() {
                if (should_enable)
                    INLINE_ASSEMBLY("sti");
            }

            InterruptGuard(const InterruptGuard&) = delete;
            InterruptGuard& operator=(const InterruptGuard&) = delete;
            InterruptGuard(InterruptGuard&&) = delete;
            InterruptGuard& operator=(InterruptGuard&&) = delete;

        private:
            bool should_enable;
    };
}