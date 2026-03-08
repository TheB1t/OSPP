#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/trivial.hpp>

namespace kstd {
    class InterruptGuard : public NonTransferable {
        public:
            InterruptGuard() : should_enable(false) {
                uint32_t flags;

                __asm__ volatile (
                     "pushfl\n"
                     "popl %0\n"
                     "cli\n"
                     : "=r" (flags)
                     :
                     : "memory"
                );

                if (flags & (1 << 9))
                    should_enable = true;
            }

            ~InterruptGuard() {
                if (should_enable)
                    __asm__ volatile ("sti");
            }

        private:
            bool should_enable;
    };
}