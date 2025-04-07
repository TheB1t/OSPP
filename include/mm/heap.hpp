#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cllist.hpp>

#define HEAP_START          0x90000000
#define HEAP_MIN_SIZE       0x100000

typedef struct {
	uint32_t			startAddr;
	uint32_t			endAddr;
	uint32_t			maxAddr;
	uint16_t			perms;
	struct	list_head	head;
} Heap_t;

typedef	struct {
	uint32_t			prevFoot;
	uint32_t			head;
	struct	list_head	list;
} HeapChunk_t;

struct Heap {
	uint32_t			startAddr;
	uint32_t			endAddr;
	uint32_t			maxAddr;
	uint16_t			perms;
	struct	list_head	head;

	void create(uint32_t start, uint32_t size, uint32_t max, uint16_t perms);
	void* alloc(uint32_t size);
	void* palignedAlloc(uint32_t size);
	void free(void* p);

	uint32_t malloc(uint32_t size, uint8_t align);
	void mfree(void* p);

	HeapChunk_t* findSmallestChunk(size_t size);
	void insertChunk(HeapChunk_t* chunk);
	void removeChunk(HeapChunk_t* chunk);


	void expand(size_t newSize);
	size_t contract(size_t newSize);
};

uint32_t heap_malloc(Heap* heap, uint32_t size, uint8_t align);
void heap_free(Heap* heap, void* addr);