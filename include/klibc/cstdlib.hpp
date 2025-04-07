#pragma once

#include <mm/heap.hpp>

extern Heap kernel_heap;

void* kmalloc(uint32_t size, bool align);
void kfree(void *ptr);