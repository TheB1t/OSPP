#include <klibcpp/cstdlib.hpp>
#include <klibabi/kapi.hpp>
#include <log.hpp>

void* operator new(size_t size) {
    if (api.kmalloc)
        return api.kmalloc(size, 0);

    kstd::panic("kmalloc not found");
    return (void*)0xDEADADDD; // Unreachable, warn suppression
}

void operator delete(void* ptr) noexcept {
    if (api.kfree)
        api.kfree(ptr);
}

void operator delete(void* ptr, size_t size) noexcept {
    (void)size;
    if (api.kfree)
        api.kfree(ptr);
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    operator delete(ptr, size);
}

void* operator new(size_t size, std::align_val_t alignment) {
    if (api.kmalloc)
        return api.kmalloc(size, (uint32_t)alignment > 0);

    kstd::panic("kmalloc not found");
    return (void*)0xDEADADDD; // Unreachable, warn suppression
}

void operator delete(void* ptr, std::align_val_t alignment) noexcept {
    (void)alignment;
    if (api.kfree)
        api.kfree(ptr);
}

void* operator new[](size_t size, std::align_val_t alignment) {
    return operator new(size, alignment);
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

void operator delete(void* ptr, uint32_t, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

namespace kstd {
    void _assert(bool cond, const char* cond_s, const char* file, int line) {
        if (!cond) {
            LOG_FATAL("Assertion failed: %s\n    File: %s\n    Line: %d\n", cond_s, file, line);
            __unreachable();
        }
    }
}

__extern_c {
    void __cxa_pure_virtual() {
        kstd::panic("pure virtual call");
    }

    static void udivmod_64(uint64_t a, uint64_t b, uint64_t* q, uint64_t* r) {
        if (b == 0) {
            kstd::panic("Division by zero in udivmod_64");
            /*
            if (q) *q = (uint64_t)-1;
            if (r) *r = a;
            return;
            */
        }

        uint64_t quotient = 0;
        uint64_t rem      = 0;
        for (int i = 63; i >= 0; --i) {
            rem = (rem << 1) | ((a >> i) & 1ULL);
            if (rem >= b) {
                rem      -= b;
                quotient |= (1ULL << i);
            }
        }
        if (q) *q = quotient;
        if (r) *r = rem;
    }

    uint64_t __udivdi3(uint64_t a, uint64_t b) {
        uint64_t q;
        udivmod_64(a, b, &q, nullptr);
        return q;
    }

    uint64_t __umoddi3(uint64_t a, uint64_t b) {
        uint64_t r;
        udivmod_64(a, b, nullptr, &r);
        return r;
    }

    void putc(char c) {
        if (api.putc)
            api.putc(c);
    }

    void puts(const char* s) {
        if (!api.putc)
            return;

        while (*s)
            api.putc(*s++);
    }

    char getc() {
        if (api.getc)
            return api.getc();

        return '\0';
    }
}
