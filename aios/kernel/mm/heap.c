#include "heap.h"
#include "pmm.h"

// Simple free-list heap allocator
// Each allocation has a header describing its size and state

typedef struct block_header {
    unsigned int size;       // Size of data (not including header)
    unsigned int free;       // 1 = free, 0 = used
    struct block_header* next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)
#define HEAP_START  0x500000
#define HEAP_SIZE   0x100000  // 1MB initial heap

static block_header_t* heap_head = 0;

void heap_init() {
    heap_head = (block_header_t*)HEAP_START;
    heap_head->size = HEAP_SIZE - HEADER_SIZE;
    heap_head->free = 1;
    heap_head->next = 0;
}

// Split a block if it has enough space for another allocation
static void split_block(block_header_t* block, unsigned int size) {
    if (block->size > size + HEADER_SIZE + 16) {
        block_header_t* new_block = (block_header_t*)((unsigned char*)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

void* kmalloc(unsigned int size) {
    if (!size) return 0;

    // Align to 4 bytes
    size = (size + 3) & ~3;

    block_header_t* current = heap_head;
    while (current) {
        if (current->free && current->size >= size) {
            split_block(current, size);
            current->free = 0;
            return (void*)((unsigned char*)current + HEADER_SIZE);
        }
        current = current->next;
    }
    return 0;  // Out of heap
}

void kfree(void* ptr) {
    if (!ptr) return;

    block_header_t* block = (block_header_t*)((unsigned char*)ptr - HEADER_SIZE);
    block->free = 1;

    // Merge with next block if it's also free (coalescing)
    while (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
}
