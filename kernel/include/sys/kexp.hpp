#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/hash.hpp>

namespace kexp {
    struct Entry {
        uint64_t name_hash;
        const void* addr;
    } __packed;

    Entry* lookup(const char* name);
}

__extern_c symbol __kexp_start;
__extern_c symbol __kexp_end;

#define EXPORT_SYMBOL(NAME_LIT, SYM)                                             \
    __section(.kexp) __used static const kexp::Entry __kexp_f_##SYM =            \
        kexp::Entry{ kstd::hash_fold32_cstr((NAME_LIT)), (const void*)(&SYM) }

#define EXPORT_SYMBOL_PTR(NAME_LIT, PTR)                                         \
    __section(.kexp) __used static const kexp::Entry __kexp_f_##PTR =            \
        kexp::Entry{ kstd::hash_fold32_cstr((NAME_LIT)), (const void*)(PTR) }

