/* 
    This source was taken from osdev.org

	Was modified by Bit.
*/

#include <icxxabi.hpp>

#if defined(__cplusplus)
extern "C" {
#endif

atexit_func_entry_t __atexit_funcs[ATEXIT_MAX_FUNCS];
uarch_t __atexit_func_count = 0;

void *__dso_handle = 0; //Attention! Optimally, you should remove the '= 0' part and define this in your asm script.

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso) {
	if (__atexit_func_count >= ATEXIT_MAX_FUNCS) 
        return -1;

	__atexit_funcs[__atexit_func_count].destructor_func = f;
	__atexit_funcs[__atexit_func_count].obj_ptr = objptr;
	__atexit_funcs[__atexit_func_count].dso_handle = dso;
	__atexit_func_count++;

	return 0; /*I would prefer if functions returned 1 on success, but the ABI says...*/
};

void __cxa_finalize(void *f) {
    uarch_t i = __atexit_func_count;

    if (!f) {
        while (i--) {
            auto fn  = __atexit_funcs[i].destructor_func;
            auto obj = __atexit_funcs[i].obj_ptr;

            __atexit_funcs[i].destructor_func = nullptr;
            __atexit_funcs[i].obj_ptr = nullptr;

            if (fn) {
                fn(obj);
            }
        }
        __atexit_func_count = 0;
        return;
    }

    for (uarch_t j = __atexit_func_count; j-- > 0; ) {
        if (__atexit_funcs[j].destructor_func == f) {
            auto obj = __atexit_funcs[j].obj_ptr;
            __atexit_funcs[j].destructor_func = nullptr;
            __atexit_funcs[j].obj_ptr = nullptr;
            ((void(*)(void*))f)(obj);
        }
    }

    uarch_t w = 0;
    for (uarch_t r = 0; r < __atexit_func_count; ++r) {
        if (__atexit_funcs[r].destructor_func) {
            if (w != r) __atexit_funcs[w] = __atexit_funcs[r];
            ++w;
        }
    }
    __atexit_func_count = w;
};

int __cxa_guard_acquire (__guard *g)  {
	return !*(char *)(g);
}

void __cxa_guard_release (__guard *g) {
	*(char *)g = 1;
}

void __cxa_guard_abort (__guard *) {

}

#if defined(__cplusplus)
}
#endif