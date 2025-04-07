#include <klibcpp/cstdlib.hpp>
#include <klibcpp/kstd.hpp>

Heap kernel_heap;

void* kmalloc(uint32_t size, bool align = 0) {
    return (void*)heap_malloc(&kernel_heap, size, align);
}

void kfree(void* ptr) {
    heap_free(&kernel_heap, ptr);
}

void* operator new(size_t size) {
    void* ptr = kmalloc(size);
    if (!ptr)
        kstd::panic("Out of memory in operator new");
    return ptr;
}

void operator delete(void* ptr) noexcept {
    kfree(ptr);
}

void operator delete(void* ptr, size_t size) noexcept {
    (void)size;
    kfree(ptr);
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

void* operator new(size_t size, align_val_t alignment) {
    // TODO: Implement proper alignment support, for now we align to page size
    void* ptr = kmalloc(size, (uint32_t)alignment > 0);
    if (!ptr)
        kstd::panic("Aligned memory allocation failed");
    return ptr;
}

void operator delete(void* ptr, align_val_t alignment) noexcept {
    (void)alignment;
    kfree(ptr);
}

void* operator new[](size_t size, align_val_t alignment) {
    return operator new(size, alignment);
}

void operator delete[](void* ptr, align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}