#pragma once

#define BIT(n)                (1 << (n))

#define SET_MASK(val, mask)   ((val) |=   (mask))
#define CLEAR_MASK(val, mask) ((val) &=  ~(mask))
#define APPLY_MASK(val, mask) ((val) &    (mask))
#define TEST_MASK(val, mask)  (APPLY_MASK(val, mask) == (mask))

#define SET_BIT(val, bit)     SET_MASK(val, BIT(bit))
#define CLEAR_BIT(val, bit)   CLEAR_MASK(val, BIT(bit))
#define TEST_BIT(val, bit)    TEST_MASK(val, BIT(bit))

#define NULL                  0

#define UINT32_MAX            ((uint32_t)-1)

#define STR2(x)               #x
#define STR(x)                STR2(x)

#define __wait_hw_int         __asm__ volatile("pause")
#define __hlt                 __asm__ volatile("hlt")
#define __pause               __asm__ volatile("pause")

#define __attr(x)             __attribute__((__ ## x ## __))
#define __attr2(x, y)         __attribute__((__ ## x ## __(y)))

#define __section(name)       __attr2(section, #name)
#define __aligned(n)          __attr2(aligned, n)

#define __packed              __attr(packed)
#define __noreturn            __attr(noreturn)
#define __isr                 __attr(interrupt)
#define __weak                __attr(weak)
#define __used                __attr(used)
#define __unused              __attr(unused)
#define __pure                __attr(pure)
#define __noinline            __attr(noinline)
#define __always_inline       __attr(always_inline)
#define __interrupt           __attr(interrupt)
#define __naked               __attr(naked)
#define __cdecl               __attr(cdecl)

#define __cli()               __asm__ volatile("cli")
#define __sti()               __asm__ volatile("sti")

#define __bochs_brk()         __asm__ volatile("xchg %bx, %bx")
#define __unreachable()       for(;;); __builtin_unreachable()

#define __offsetof(t, m)      __builtin_offsetof(t, m)

#define __extern_c            extern "C"
#define __extern_asm          __extern_c

typedef void*              symbol[];

typedef	unsigned long long uint64_t;
typedef          long long int64_t;
typedef	unsigned int       uint32_t;
typedef          int       int32_t;
typedef	unsigned short     uint16_t;
typedef          short     int16_t;
typedef	unsigned char      uint8_t;
typedef          char      int8_t;
typedef uint32_t           size_t;

enum class align_val_t : size_t {};
