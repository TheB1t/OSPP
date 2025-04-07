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

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);
void __cxa_finalize(void *f);

#if defined(__cplusplus)
}
#endif