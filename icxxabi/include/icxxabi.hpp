/* 
    This source was taken from osdev.org
*/

#pragma once

#define ATEXIT_MAX_FUNCS	128
	
#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned uarch_t;

typedef struct {
	/*
	* Each member is at least 4 bytes large. Such that each entry is 12bytes.
	* 128 * 12 = 1.5KB exact.
	**/
	void (*destructor_func)(void *);
	void *obj_ptr;
	void *dso_handle;
} atexit_func_entry_t;

__extension__ typedef int __guard __attribute__((mode(__DI__)));

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);
void __cxa_finalize(void *f);

int __cxa_guard_acquire (__guard *);
void __cxa_guard_release (__guard *);
void __cxa_guard_abort (__guard *);

#if defined(__cplusplus)
}
#endif