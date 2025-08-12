#pragma once

#include <klibcpp/cstdint.hpp>

static inline uint64_t read_msr(uint32_t msr) {
    uint32_t eax, edx;
    __asm__ __volatile__(
        "movl %2, %%ecx\n"
        "rdmsr\n"
        : "=a"(eax), "=d"(edx)
        : "r"(msr)
        : "ecx"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline void write_msr(uint32_t msr, uint64_t data) {
    uint32_t low = data & 0xFFFFFFFF;
    uint32_t high = data >> 32;
    __asm__ __volatile__(
        "movl %0, %%ecx\n"
        "movl %1, %%eax\n"
        "movl %2, %%edx\n"
        "wrmsr\n"
        :
        : "r"(msr), "r"(low), "r"(high)
        : "eax", "ecx", "edx"
    );
}

static inline uint64_t read_tsc(void) {
    uint32_t eax, edx;
    __asm__ __volatile__(
        "rdtsc\n"
        : "=a"(eax), "=d"(edx)
        :
        :
    );
    return ((uint64_t)edx << 32) | eax;
}