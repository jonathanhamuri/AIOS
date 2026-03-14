#ifndef USERSPACE_H
#define USERSPACE_H

// Segment selectors
#define SEG_KERNEL_CODE  0x08
#define SEG_KERNEL_DATA  0x10
#define SEG_USER_CODE    0x18
#define SEG_USER_DATA    0x20
#define SEG_TSS          0x28

// TSS structure (minimal)
typedef struct {
    unsigned int prev_tss;
    unsigned int esp0;      // kernel stack pointer
    unsigned int ss0;       // kernel stack segment
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax,ecx,edx,ebx,esp,ebp,esi,edi;
    unsigned int es,cs,ss,ds,fs,gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed)) tss_t;

void userspace_init();
void jump_to_usermode(unsigned int entry, unsigned int user_stack);
int  userspace_exec(void* code, unsigned int size);

extern tss_t kernel_tss;

#endif
