#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/utility.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/iguard.hpp>
#include <klibabi/kapi.hpp>

inline void* operator new(size_t, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept {}

void* operator new(size_t size);
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void* operator new[](size_t size);
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;
void* operator new(size_t size, align_val_t alignment);
void operator delete(void* ptr, align_val_t alignment) noexcept;
void* operator new[](size_t size, align_val_t alignment);
void operator delete[](void* ptr, align_val_t alignment) noexcept;

namespace kstd {
    template<typename ...Args>
    inline void panic(const char* fmt, Args&&... args) {
        static char buf[128];
        snprintf(buf, sizeof(buf), fmt, kstd::forward<Args>(args)...);

        if (api.panic)
            api.panic(buf);

        __unreachable();
    }

    void _assert(bool cond, const char* cond_s, const char* file, int line);

    template<int N>
    inline void trigger_interrupt() {
        INLINE_ASSEMBLY("int %0" : : "i"(N) : "memory");
    }

    inline void set_stack_pointer(uint32_t ptr) {
        INLINE_ASSEMBLY("mov %0, %%esp" : : "r"(ptr) : "memory");
    }

    inline uint32_t get_stack_pointer() {
        uint32_t ptr;
        INLINE_ASSEMBLY("mov %%esp, %0" : "=r"(ptr) : : "memory");
        return ptr;
    }
}

__extern_c {
    void __cxa_pure_virtual();
    uint64_t __udivdi3(uint64_t a, uint64_t b);
    uint64_t __umoddi3(uint64_t a, uint64_t b);

    void putc(char c);
    void puts(char* s);
    char getc();
}