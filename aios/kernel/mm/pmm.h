#ifndef PMM_H
#define PMM_H

#define PAGE_SIZE 4096
#define PMM_BITMAP_START 0x200000
#define PMM_MEMORY_START 0x400000
#define PMM_MEMORY_SIZE  0x1000000

void  pmm_init();
void* pmm_alloc_page();
void  pmm_free_page(void* addr);
int   pmm_free_pages();

#endif
