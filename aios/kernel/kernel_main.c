#include "mm/pmm.h"
#include "mm/heap.h"
#include "terminal/terminal.h"
#include "terminal/ai_input.h"
#include "syscall/syscall.h"
#include "process/process.h"

extern void setup_idt();
extern void idt_install_syscall();

static void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}
static unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static void serial_init() {
    outb(0x3F9,0x00); outb(0x3FB,0x80); outb(0x3F8,0x01);
    outb(0x3F9,0x00); outb(0x3FB,0x03); outb(0x3FA,0xC7);
}
static void serial_putchar(char c) {
    while (!(inb(0x3FD)&0x20)); outb(0x3F8,c);
    if (c=='\n'){while(!(inb(0x3FD)&0x20));outb(0x3F8,'\r');}
}
static void sp(const char* s){while(*s)serial_putchar(*s++);}

static unsigned char sc2a[] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,'\\','z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0
};

void kernel_main() {
    serial_init();
    pmm_init();
    heap_init();
    terminal_init();
    syscall_init();
    setup_idt();
    idt_install_syscall();
    process_init();

    sp("AIOS kernel ready\n");

    syscall_dispatch(SYS_PRINT,(unsigned int)"Syscall OK\n",
                     MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK),0);
    syscall_dispatch(SYS_AI,(unsigned int)"hello",0,0);

    terminal_render_prompt();

    while (1) {
        if (!(inb(0x64)&0x01)) continue;
        unsigned char sc = inb(0x60);
        if (sc&0x80) continue;
        if (sc==0x0E){terminal_handle_key('\b');continue;}
        if (sc<sizeof(sc2a)){char c=sc2a[sc];if(c)terminal_handle_key(c);}
    }
}
