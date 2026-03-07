#pragma once

#include <klibcpp/cstdint.hpp>


__extern_c {
    struct KernelAPI {
        __cdecl void*    (*kmalloc)(uint32_t, bool);
        __cdecl void     (*kfree)(void*);

        __cdecl uint32_t (*get_ticks)();
        __cdecl void     (*panic)(const char*);

        /*
            TOOD:
                Kind of 'stdin/stdout'...
                Need to replace it with some reliable mechanism
        */
        __cdecl void     (*putc)(char);
        __cdecl char     (*getc)();
    } __packed;
}

__extern_c KernelAPI api;