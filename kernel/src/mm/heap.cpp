#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <klibcpp/kstd.hpp>
#include <log.hpp>

#define SIZE_T_SIZE         (sizeof(size_t))
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define SIZE_T_ZERO         ((size_t)0)
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define SIZE_T_FOUR         ((size_t)4)

#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define RESERV_BIT          (SIZE_T_FOUR)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define FLAG_BITS           (INUSE_BITS|RESERV_BIT)

#define HEAPCHUNK_SIZE      (sizeof(HeapChunk_t))
#define KMALLOC_ALIGNMENT   ((size_t)(2 * sizeof(void*)))

#define KMALLOC_ALIGN_MASK  (KMALLOC_ALIGNMENT - SIZE_T_ONE)
#define MIN_CHUNK_SIZE      ((HEAPCHUNK_SIZE + KMALLOC_ALIGN_MASK) & ~KMALLOC_ALIGN_MASK)
#define CHUNK_OVERHEAD      (SIZE_T_SIZE)

#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

#define chunksize(ptr)      ((ptr)->head & ~(FLAG_BITS))

#define get_head(heap)      (&(heap)->head)

#define chunk2mem(ptr)      ((void*)((char*)(ptr) + TWO_SIZE_T_SIZES))
#define mem2chunk(ptr)      ((HeapChunk_t*)((char*)(ptr) - TWO_SIZE_T_SIZES))

#define pinuse(p)           ((p)->head & PINUSE_BIT)
#define cinuse(p)           ((p)->head & CINUSE_BIT)
#define ok_address(p, h)    (((size_t)(p) >= (h)->startAddr) && ((size_t)(p) < (h)->endAddr))
#define ok_inuse(p)         (((p)->head & INUSE_BITS) != PINUSE_BIT)

#define pad_request(req)    (((req) + CHUNK_OVERHEAD + KMALLOC_ALIGN_MASK) & ~KMALLOC_ALIGN_MASK)

#define request2size(req)   (((req) < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(req))

#define chunk_plus_offset(p, s)     ((HeapChunk_t*)((char*)(p) + (s)))
#define chunk_minus_offset(p, s)    ((HeapChunk_t*)((char*)(p) - (s)))

#define set_pinuse(p)       ((p)->head |= PINUSE_BIT)
#define clear_pinuse(p)     ((p)->head &= ~PINUSE_BIT)

#define set_inuse(p,s) \
    ((p)->head = (((p)->head & PINUSE_BIT)|CINUSE_BIT|(s)), \
    (chunk_plus_offset(p, s))->head |= PINUSE_BIT)

#define set_size_and_pinuse(p, s) \
    ((p)->head = ((s)|PINUSE_BIT))

#define set_foot(p, s)      (chunk_plus_offset(p, s)->prevFoot = (s))

uint32_t heap_malloc(Heap* heap, uint32_t size, uint8_t align) {
    if (heap) {
        uint32_t addr;
        if (align)
            addr = (uint32_t)heap->palignedAlloc(size);
        else
            addr = (uint32_t)heap->alloc(size);

        return addr;
    }

    return 0;
}

void heap_free(Heap* heap, void* addr) {
	if (heap)
		heap->free(addr);
}

HeapChunk_t* Heap::findSmallestChunk(size_t size) {
	HeapChunk_t* tmp = 0;
	struct list_head* iter;
	list_for_each(iter, get_head(this)) {
		tmp = list_entry(iter, HeapChunk_t, list);
		if (chunksize(tmp) >= size)
			break;
	}

	if (iter == get_head(this))
		return 0;

	return tmp;
}

void Heap::insertChunk(HeapChunk_t* chunk) {
	if (list_empty(get_head(this)))
		list_add_tail(&(chunk->list), get_head(this));
	else {
		struct list_head* iter;
		size_t csize = chunksize(chunk);
		list_for_each(iter, get_head(this)) {
			HeapChunk_t* tmp = list_entry(iter, HeapChunk_t, list);
			if (chunksize(tmp) >= csize)
				break;
		}

		list_add_tail(&(chunk->list), iter);
	}
}

void Heap::removeChunk(HeapChunk_t* chunk) {
	__list_del_entry(&(chunk->list));
}

void Heap::create(uint32_t start, uint32_t size, uint32_t max, uint16_t perms) {
	uint32_t end = start + size;
	assert(start % mm::PAGE_SIZE == 0);
	assert(end % mm::PAGE_SIZE == 0);

	this->startAddr	= start;
	this->endAddr	= end;
	this->maxAddr	= max;
	this->perms		= perms;
	INIT_LIST_HEAD(get_head(this));

	uint32_t phys = mm::pmm::alloc_pages(size / mm::PAGE_SIZE);
	mm::vmm::map_pages(start, phys, size / mm::PAGE_SIZE, this->perms | mm::Flags::Present);

	HeapChunk_t* hole = (HeapChunk_t*)start;
	hole->prevFoot = 0;
	set_size_and_pinuse(hole, size);
	insertChunk(hole);
}

void Heap::expand(size_t newSize) {
	assert(newSize > this->endAddr - this->startAddr);

	if (newSize % mm::PAGE_SIZE) {
		newSize &= -mm::PAGE_SIZE;
		newSize += mm::PAGE_SIZE;
	}

	assert(this->startAddr + newSize <= this->maxAddr);

	uint32_t oldEnd = this->endAddr;
    this->endAddr = this->startAddr + newSize;

    size_t size = this->endAddr - oldEnd;
    uint32_t phys = mm::pmm::alloc_pages(size / mm::PAGE_SIZE);

	mm::vmm::map_pages(oldEnd, phys, size / mm::PAGE_SIZE, this->perms | mm::Flags::Present);
}

size_t Heap::contract(size_t newSize) {
	assert(newSize < this->endAddr - this->startAddr);

	if (newSize % mm::PAGE_SIZE) {
		newSize &= -mm::PAGE_SIZE;
		newSize += mm::PAGE_SIZE;
	}

	if (newSize < HEAP_MIN_SIZE)
		newSize = HEAP_MIN_SIZE;

	uint32_t oldEnd = this->endAddr;
    this->endAddr = this->startAddr + newSize;

    size_t size = oldEnd - this->endAddr;
	uint32_t phys = mm::vmm::virt_to_phys(this->endAddr);
	if (phys == 0xFFFFFFFF)
		LOG_WARN("[heap] trying to free unmapped memory! addr: 0x%08x\n", this->endAddr);

	mm::vmm::unmap_pages(this->endAddr, size / mm::PAGE_SIZE);
	mm::pmm::free_pages(phys, size / mm::PAGE_SIZE);

	return newSize;
}

void* Heap::alloc(size_t size) {
	size_t nb = request2size(size);

	HeapChunk_t* hole = findSmallestChunk(nb);
	if (hole == 0) {
		size_t oldSize = this->endAddr - this->startAddr;
		size_t oldEndAddr = this->endAddr;
		expand(oldSize + nb);

		struct list_head* tmp = 0;
		if (!list_empty(get_head(this))) {
			struct list_head* iter;
			list_for_each(iter, get_head(this)) {
				if (iter > tmp)
					tmp = iter;
			}
		}

		HeapChunk_t* topChunk = 0;
		size_t csize = 0;

		if (tmp) {
			topChunk = list_entry(tmp, HeapChunk_t, list);
			if (!pinuse(topChunk) && cinuse(topChunk))
				goto _Lassert;

			size_t oldCsize = chunksize(topChunk);
			if ((size_t)chunk_plus_offset(topChunk, oldCsize) != oldEndAddr)
				topChunk = 0;
			else {
				removeChunk(topChunk);
				csize = this->endAddr - (size_t)topChunk;
			}
		}

		if (topChunk == 0) {
			topChunk = (HeapChunk_t*)oldEndAddr;
			csize = this->endAddr - oldEndAddr;
		}

		set_size_and_pinuse(topChunk, csize);
		insertChunk(topChunk);

		return alloc(size);
	}

	assert(chunksize(hole) >= nb);

	removeChunk(hole);

	if (chunksize(hole) >= nb + MIN_CHUNK_SIZE) {
		size_t nsize = chunksize(hole) - nb;

		HeapChunk_t* _new = chunk_plus_offset(hole, nb);
		_new->head = nsize;

		insertChunk(_new);
	} else
		nb = chunksize(hole);

	set_inuse(hole, nb);

	return chunk2mem(hole);

_Lassert:
	kstd::panic("(alloc) Memory Corrupt");
	return nullptr;
}

void Heap::free(void* ptr) {
	if (ptr == 0)
		return;

	HeapChunk_t* block = mem2chunk(ptr);
	size_t size = chunksize(block);
	HeapChunk_t* next = chunk_plus_offset(block, size);

	if (!ok_address(ptr, this))
		goto _Lassert;

	if (!ok_inuse(block))
		goto _Lassert;

	if (!pinuse(block)) {
		size_t prevsize = block->prevFoot;
		HeapChunk_t* prev = chunk_minus_offset(block, prevsize);

		if (!ok_address(prev, this))
			goto _Lassert;

		size += prevsize;
		block = prev;

		removeChunk(block);
	}

	if (!ok_address(next, this)) {
		if (!pinuse(next))
			goto _Lassert2;

		if (cinuse(next))
			clear_pinuse(next);
		else {
			size_t nextsize = chunksize(next);

			if ((size_t)chunk_plus_offset(next, nextsize) > this->endAddr)
				goto _Lassert2;

			removeChunk(next);

			size += nextsize;
		}
	}

	if ((size_t)chunk_plus_offset(block, size) == this->endAddr) {
		size_t oldSize = this->endAddr - this->startAddr;
		size_t newSize = (size_t)block - this->startAddr;

		if (newSize % mm::PAGE_SIZE)
			newSize += MIN_CHUNK_SIZE;

		newSize = contract(newSize);

		if (size > oldSize - newSize)
			size -= oldSize - newSize;
		else {
			size = 0;
			block = 0;
		}
	} else
		set_foot(block, size);

	if (block && size) {
		set_size_and_pinuse(block, size);
		insertChunk(block);
	}

	return;

_Lassert2:
	insertChunk(block);
_Lassert:
	kstd::panic("(free) Memory Corrupt");
}

void* Heap::palignedAlloc(size_t size) {
	size_t nb = request2size(size);
	size_t req = nb + mm::PAGE_SIZE + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;

	void* mem = alloc(req);
	if (mem) {
		HeapChunk_t* chunk = mem2chunk(mem);
		if ((size_t)mem % mm::PAGE_SIZE) {
			char* br = (char*)mem2chunk(((size_t)mem & -mm::PAGE_SIZE) + mm::PAGE_SIZE);
			char* pos = ((size_t)(br - (char*)(chunk)) >= MIN_CHUNK_SIZE) ? br : br + mm::PAGE_SIZE;

			HeapChunk_t* newChunk = (HeapChunk_t*)pos;
			size_t leadsize = pos - (char*)(chunk);
			size_t newSize = chunksize(chunk) - leadsize;

			set_inuse(newChunk, newSize);

			set_size_and_pinuse(chunk, leadsize);
			newChunk->prevFoot = leadsize;

			insertChunk(chunk);

			chunk = newChunk;
		}

		size_t csize = chunksize(chunk);
		if (csize > nb + MIN_CHUNK_SIZE) {
			size_t remainderSize = csize - nb;
			HeapChunk_t* remainder = chunk_plus_offset(chunk, nb);
			set_inuse(chunk, nb);
			set_inuse(remainder, remainderSize);
			free(chunk2mem(remainder));
		}

		mem = chunk2mem(chunk);
		assert(chunksize(chunk) >= nb);
		assert(((size_t)mem % mm::PAGE_SIZE) == 0);
		assert(cinuse(chunk));
	}

	return mem;
}