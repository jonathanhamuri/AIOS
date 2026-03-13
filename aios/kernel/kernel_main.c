#include "mm/pmm.h"
#include "mm/heap.h"
#include "terminal/terminal.h"
#include "terminal/ai_input.h"
#include "syscall/syscall.h"

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
    serial_init();         sp("1: serial OK\n");
    pmm_init();            sp("2: pmm OK\n");
    heap_init();           sp("3: heap OK\n");
    terminal_init();       sp("4: terminal OK\n");
    syscall_init();        sp("5: syscall_init OK\n");
    setup_idt();           sp("6: setup_idt OK\n");
    idt_install_syscall(); sp("7: idt_install OK\n");

    // Call syscall_dispatch DIRECTLY - no int 0x80 yet
    sp("8: testing direct dispatch\n");
    syscall_dispatch(SYS_PRINT,(unsigned int)"DIRECT DISPATCH OK\n",
                     MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK),0);
    sp("9: direct dispatch OK\n");

    syscall_dispatch(SYS_AI,(unsigned int)"hello",0,0);
    sp("10: AI dispatch OK\n");

    terminal_render_prompt();
    sp("11: READY\n");

    while (1) {
        if (!(inb(0x64)&0x01)) continue;
        unsigned char sc = inb(0x60);
        if (sc&0x80) continue;
        if (sc==0x0E){terminal_handle_key('\b');continue;}
        if (sc<sizeof(sc2a)){char c=sc2a[sc];if(c)terminal_handle_key(c);}
    }
}
