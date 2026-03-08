#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/spinlock.hpp>
#include <klibcpp/cllist.hpp>
#include <mm/layout.hpp>

inline constexpr uint32_t HEAP_START    = mm::layout::virt::KERNEL_HEAP_BASE;
inline constexpr uint32_t HEAP_MIN_SIZE = mm::layout::virt::KERNEL_HEAP_INITIAL_SIZE;
inline constexpr uint32_t HEAP_MAX_ADDR = mm::layout::virt::KERNEL_HEAP_BASE + mm::layout::virt::KERNEL_HEAP_SIZE;

typedef	struct {
    uint32_t          prevFoot;
    uint32_t          head;
    struct	list_head list;
} HeapChunk_t;

class Heap {
    public:
        Heap(uint32_t start, uint32_t size, uint32_t max, uint16_t perms);
        ~Heap() = default;

        void*        alloc(uint32_t size);
        void*        palignedAlloc(uint32_t size);
        void         free(void* p);

        uint32_t     malloc(uint32_t size, uint8_t align);
        void         mfree(void* p);
        void         expand(size_t newSize);
        size_t       contract(size_t newSize);

    private:
        HeapChunk_t* findSmallestChunk(size_t size);
        void         insertChunk(HeapChunk_t* chunk);
        void         removeChunk(HeapChunk_t* chunk);

        void*        alloc_unlocked(size_t size);
        void*        palignedAlloc_unlocked(size_t size);
        void         free_unlocked(void* p);
        void         expand_unlocked(size_t newSize);
        size_t       contract_unlocked(size_t newSize);

        uint32_t startAddr;
        uint32_t endAddr;
        uint32_t maxAddr;
        uint16_t perms;
        kstd::SpinLock lock;
        struct	list_head head;
};

uint32_t heap_malloc(Heap* heap, uint32_t size, uint8_t align);
void     heap_free(Heap* heap, void* addr);
