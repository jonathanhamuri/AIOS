#include "userspace.h"
#include "../terminal/terminal.h"
#include "../mm/heap.h"

tss_t kernel_tss;

// Kernel stack for TSS (when interrupt fires in user mode)
static unsigned char kernel_stack[8192];

static void outb(unsigned short port, unsigned char val){
    __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port));
}

// Write TSS descriptor into GDT slot 5 (offset 0x28)
static void gdt_set_tss(unsigned int base, unsigned int limit){
    // GDT is at our_gdt - we need to write slot at offset 0x28
    extern unsigned char our_gdt[];
    unsigned char* entry = our_gdt + 0x28;
    entry[0] = limit & 0xFF;
    entry[1] = (limit >> 8) & 0xFF;
    entry[2] = base & 0xFF;
    entry[3] = (base >> 8) & 0xFF;
    entry[4] = (base >> 16) & 0xFF;
    entry[5] = 0x89;  // present, ring0, TSS available
    entry[6] = 0x00;
    entry[7] = (base >> 24) & 0xFF;
}

void userspace_init(){
    // Set up TSS
    unsigned int tss_base  = (unsigned int)&kernel_tss;
    unsigned int tss_limit = sizeof(tss_t) - 1;

    // Kernel stack top
    unsigned int kstack = (unsigned int)kernel_stack + sizeof(kernel_stack);

    // Fill TSS
    for(unsigned int i=0;i<sizeof(tss_t);i++)
        ((unsigned char*)&kernel_tss)[i]=0;

    kernel_tss.ss0   = SEG_KERNEL_DATA;
    kernel_tss.esp0  = kstack;
    kernel_tss.cs    = SEG_USER_CODE | 3;
    kernel_tss.ss    = SEG_USER_DATA | 3;
    kernel_tss.ds    = SEG_USER_DATA | 3;
    kernel_tss.es    = SEG_USER_DATA | 3;
    kernel_tss.fs    = SEG_USER_DATA | 3;
    kernel_tss.gs    = SEG_USER_DATA | 3;
    kernel_tss.iomap_base = sizeof(tss_t);

    // Install TSS in GDT
    gdt_set_tss(tss_base, tss_limit);

    // Load TSS
    __asm__ volatile("ltr %0" :: "r"((unsigned short)SEG_TSS));

    terminal_print_color("User space       : OK (Ring 3 ready)\n",
        MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
}

// Jump to user mode at given entry point
void jump_to_usermode(unsigned int entry, unsigned int user_stack){
    // Set up iret frame for ring3
    // Stack layout for iret: SS, ESP, EFLAGS, CS, EIP
    unsigned int user_cs = SEG_USER_CODE | 3;  // RPL=3
    unsigned int user_ds = SEG_USER_DATA | 3;  // RPL=3
    unsigned int user_ss = SEG_USER_DATA | 3;

    // Load user data segments
    __asm__ volatile(
        "movw %w0, %%ds\n"
        "movw %w0, %%es\n"
        "movw %w0, %%fs\n"
        "movw %w0, %%gs\n"
        :: "r"((unsigned short)user_ds)
    );

    // iret to user mode
    __asm__ volatile(
        "push %0\n"       // SS
        "push %1\n"       // ESP
        "pushf\n"         // EFLAGS
        "pop %%eax\n"
        "or $0x200, %%eax\n"  // enable interrupts in user mode
        "push %%eax\n"
        "push %2\n"       // CS
        "push %3\n"       // EIP
        "iret\n"
        :: "r"(user_ss), "r"(user_stack),
           "r"(user_cs), "r"(entry)
        : "eax"
    );
}

// Allocate memory and copy user code, then jump to it
int userspace_exec(void* code, unsigned int size){
    // Allocate user stack (8KB)
    unsigned char* ustack = (unsigned char*)kmalloc(8192);
    if(!ustack) return -1;
    unsigned int ustop = (unsigned int)ustack + 8192;

    // Copy code to executable region
    unsigned char* ucode = (unsigned char*)kmalloc(size + 64);
    if(!ucode){ return -1; }
    for(unsigned int i=0;i<size;i++) ucode[i]=((unsigned char*)code)[i];

    terminal_print_color("[Ring3] Launching user process...\n",
        MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));

    jump_to_usermode((unsigned int)ucode, ustop);
    return 0;
}
