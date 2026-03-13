#include "mm/pmm.h"
#include "mm/heap.h"
#include "terminal/terminal.h"
#include "terminal/ai_input.h"

// Keyboard port
#define KB_PORT 0x60

static unsigned char scancode_to_ascii[] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  0,
    'q','w','e','r','t','y','u','i','o','p','[',']', '\n', 0, 'a','s',
    'd','f','g','h','j','k','l',';', 39, '`',  0,  '\\','z','x','c','v',
    'b','n','m',',','.','/',  0,  '*', 0,  ' ',  0
};

static unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static unsigned char kb_read() {
    // Wait for keyboard buffer to have data
    while (!(inb(0x64) & 1));
    return inb(KB_PORT);
}

void kernel_main() {
    // Init subsystems
    pmm_init();
    heap_init();
    terminal_init();

    // Main input loop — reads keyboard, routes through AI terminal
    while (1) {
        unsigned char sc = kb_read();

        // Ignore key releases (bit 7 set)
        if (sc & 0x80) continue;

        // Backspace
        if (sc == 0x0E) {
            terminal_handle_key('\b');
            continue;
        }

        // Translate scancode to ASCII
        if (sc < sizeof(scancode_to_ascii)) {
            char c = scancode_to_ascii[sc];
            if (c) terminal_handle_key(c);
        }
    }
}
