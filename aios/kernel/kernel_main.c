#include "mm/pmm.h"
#include "mm/heap.h"

unsigned short* vga = (unsigned short*)0xB8000;
int col = 0, row = 0;

void vga_putchar(char c, unsigned char color) {
    if (c == '\n') { row++; col = 0; return; }
    vga[row * 80 + col] = (unsigned short)c | ((unsigned short)color << 8);
    col++;
    if (col >= 80) { col = 0; row++; }
}

void vga_print(const char* s, unsigned char color) {
    while (*s) vga_putchar(*s++, color);
}

void vga_print_int(unsigned int n, unsigned char color) {
    if (n == 0) { vga_putchar('0', color); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) vga_putchar(buf[j], color);
}

void vga_print_hex(unsigned int n, unsigned char color) {
    vga_print("0x", color);
    for (int i = 28; i >= 0; i -= 4) {
        int nibble = (n >> i) & 0xF;
        vga_putchar(nibble < 10 ? '0'+nibble : 'A'+nibble-10, color);
    }
}

void kernel_main() {
    // Clear screen
    for (int i = 0; i < 80*25; i++) vga[i] = 0x0720;

    vga_print("============================================\n", 0x0A);
    vga_print("   AIOS Kernel - Phase 2 Memory Manager    \n", 0x0F);
    vga_print("============================================\n", 0x0A);

    // Init memory managers
    pmm_init();
    heap_init();

    vga_print("PMM initialized  - free pages: ", 0x0E);
    vga_print_int(pmm_free_pages(), 0x0F);
    vga_print("\n", 0x0F);

    // Test kmalloc
    vga_print("Testing kmalloc...\n", 0x0E);

    char* a = (char*)kmalloc(64);
    char* b = (char*)kmalloc(128);
    char* c = (char*)kmalloc(256);

    vga_print("  kmalloc(64)  -> ", 0x07);
    vga_print_hex((unsigned int)a, 0x0F);
    vga_print("\n", 0x0F);

    vga_print("  kmalloc(128) -> ", 0x07);
    vga_print_hex((unsigned int)b, 0x0F);
    vga_print("\n", 0x0F);

    vga_print("  kmalloc(256) -> ", 0x07);
    vga_print_hex((unsigned int)c, 0x0F);
    vga_print("\n", 0x0F);

    // Write to allocated memory
    a[0] = 'A'; a[1] = 'I'; a[2] = 'O'; a[3] = 'S'; a[4] = 0;
    vga_print("  Write to alloc: ", 0x07);
    vga_print(a, 0x0A);
    vga_print("\n", 0x0F);

    // Free and reallocate
    kfree(b);
    char* d = (char*)kmalloc(128);
    vga_print("  kfree + realloc -> ", 0x07);
    vga_print_hex((unsigned int)d, 0x0F);
    vga_print(d == b ? " (reused!) " : " (new block)", 0x0A);
    vga_print("\n", 0x0F);

    // Test PMM page allocation
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();
    vga_print("PMM alloc page1: ", 0x0E);
    vga_print_hex((unsigned int)page1, 0x0F);
    vga_print("\n", 0x0F);
    vga_print("PMM alloc page2: ", 0x0E);
    vga_print_hex((unsigned int)page2, 0x0F);
    vga_print("\n", 0x0F);

    pmm_free_page(page1);
    vga_print("PMM free page1 - free pages now: ", 0x0E);
    vga_print_int(pmm_free_pages(), 0x0F);
    vga_print("\n", 0x0F);

    vga_print("============================================\n", 0x0A);
    vga_print("Memory manager OK - Phase 2 foundation set \n", 0x0F);
    vga_print("============================================\n", 0x0A);

    while (1) {}
}
