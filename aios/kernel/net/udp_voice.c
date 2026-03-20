#include "udp_voice.h"
#include "../graphics/aios_ui.h"
#include "../terminal/terminal.h"
#include "../ai/ai_exec.h"

/* Simple UDP-style packet receive via MMIO polling */
/* For now: reads from a shared memory region that the voice bridge writes to */
/* This works in QEMU via the ISA debug port */

#define QEMU_DEBUG_PORT 0xE9

static void write_debug(const char* s){
    while(*s){ __asm__ volatile("outb %0, %1"::"a"(*s),"Nd"((unsigned short)QEMU_DEBUG_PORT)); s++; }
}

void udp_voice_init(void){
    write_debug("[AIOS] Voice UDP listener ready\n");
}

/* Called from kernel tick — checks for incoming voice packets */
void udp_voice_tick(void){
    /* Future: poll RTL8139 for UDP packets on port 7777 */
    /* For now voice bridge communicates via QEMU serial */
}
