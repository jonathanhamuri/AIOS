#include "pmm.h"

// Bitmap stored at 0x200000
// Each bit = one 4KB page of physical memory starting at 0x400000
// 0 = free, 1 = used

static unsigned char* bitmap = (unsigned char*)PMM_BITMAP_START;
static unsigned int total_pages = PMM_MEMORY_SIZE / PAGE_SIZE;

void pmm_init() {
    // Mark all pages as free
    unsigned int bitmap_bytes = total_pages / 8;
    for (unsigned int i = 0; i < bitmap_bytes; i++)
        bitmap[i] = 0x00;
}

static void bitmap_set(unsigned int page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(unsigned int page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(unsigned int page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void* pmm_alloc_page() {
    for (unsigned int i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return (void*)(PMM_MEMORY_START + i * PAGE_SIZE);
        }
    }
    return 0;  // Out of memory
}

void pmm_free_page(void* addr) {
    unsigned int page = ((unsigned int)addr - PMM_MEMORY_START) / PAGE_SIZE;
    bitmap_clear(page);
}

int pmm_free_pages() {
    int count = 0;
    for (unsigned int i = 0; i < total_pages; i++)
        if (!bitmap_test(i)) count++;
    return count;
}
