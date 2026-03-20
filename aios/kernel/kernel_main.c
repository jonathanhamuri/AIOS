#include "ai/autonomy/autonomy.h"
#include "apps/apps.h"
#include "net/discovery/discovery.h"
#include "graphics/aios_ui.h"
#include "ai/learning/learning.h"
#include "graphics/framebuffer.h"
#include "net/net.h"
#include "userspace/userspace.h"
#include "graphics/vga.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "terminal/terminal.h"
#include "terminal/ai_input.h"
#include "syscall/syscall.h"
#include "process/process.h"
#include "compiler/compiler.h"
#include "ai/ai_exec.h"
#include "ai/knowledge/kb.h"
#include "ai/knowledge/self_extend.h"
#include "disk/ata.h"
#include "disk/kbfs.h"

extern void setup_idt();
extern void idt_install_syscall();
extern void idt_install_timer();
extern void pit_init(int hz);

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

static unsigned char sc2a[] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,'\\','z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0
};
static unsigned char sc2a_shift[] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S',
    'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,' ',0
};
static int shift_held = 0;
static int ctrl_held = 0;
static int alt_held = 0;
static int caps_lock = 0;
static char clipboard[256] = {0};
static int clipboard_len = 0;

void kernel_main() {
    pmm_init();
    heap_init();
    terminal_init();
    setup_idt();
    serial_init();
    idt_install_syscall();
    idt_install_timer();
    pit_init(100);
    syscall_init();
    process_init();
    compiler_init((unsigned int)syscall_dispatch);
    terminal_print_color("Compiler         : OK\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    ai_exec_init();
    kb_init();
    self_extend_init();
    fb_init();
    net_init();
    discovery_init();
    userspace_init();
    if(1){
        if(!fb_active) vga_init();
        aios_ui_init();
    } else {
        fb_shell_init();
    }
    vga_shell_print("AIMERANCIA booting...", 2);
    ata_init();
    kbfs_init();
    kbfs_load();  // Load saved knowledge from disk on boot
    learning_init();
    scheduler_init();
    autonomy_init();

    terminal_newline();
    terminal_render_prompt();
    if(vga_active) vga_shell_prompt();
    __asm__ volatile("sti");

    while(1) {
        if(!(inb(0x64)&0x01)){
        extern unsigned int timer_ticks_bss;
        static unsigned int last_tick = 0;
        if(timer_ticks_bss != last_tick){
            last_tick = timer_ticks_bss;
            extern void scheduler_tick();
            scheduler_tick();
            scheduler_tick_check((int)timer_ticks_bss);
            autonomy_auto_tick((int)timer_ticks_bss);
            if(aios_ui_active) aios_ui_tick();
            /* Poll serial for voice commands from bridge */
            {
                static char vbuf[128];
                static int  vlen=0;
                unsigned char lsr;
                __asm__ volatile("inb %1,%0":"=a"(lsr):"Nd"((unsigned short)0x3FD));
                if(lsr & 0x01){
                    char ch;
                    __asm__ volatile("inb %1,%0":"=a"(ch):"Nd"((unsigned short)0x3F8));
                    if(ch=='\n'||ch=='\r'){
                        if(vlen>0){
                            vbuf[vlen]=0;
                            /* Show on UI */
                            aios_ui_set_status("PROCESSING...");
                            aios_ui_voice_input(vbuf);
                            /* Run through real AI */
                            ai_process_input(vbuf);
                            aios_ui_set_status("LISTENING...");
                            vlen=0;
                        }
                    } else if(vlen<127){
                        vbuf[vlen++]=ch;
                    }
                }
            }
            if(aios_ui_active) aios_ui_tick();
            if(aios_ui_active) aios_ui_tick();
        }
        continue;
    }
        unsigned char sc = inb(0x60);
        if(sc==0x2A||sc==0x36){shift_held=1;continue;}
        if(sc==0xAA||sc==0xB6){shift_held=0;continue;}
        if(sc==0x1D){ctrl_held=1;continue;}
        if(sc==0x9D){ctrl_held=0;continue;}
        if(sc==0x38){alt_held=1;continue;}
        if(sc==0xB8){alt_held=0;continue;}
        if(sc==0x3A){caps_lock=!caps_lock;continue;}
        extern void vga_shell_print(const char* s, unsigned char color);
        extern int vga_active;
        if(sc&0x80) continue;
        if(sc==0x0E||sc==0x53){
            if(aios_ui_active) aios_ui_input_backspace();
            else terminal_handle_key('\b');
            continue;
        }
        if(sc==0x1C){
            if(aios_ui_active){
                static char cb[48];
                const char* src=aios_ui_get_input();
                int ci=0;
                while(src[ci]&&ci<47){cb[ci]=src[ci];ci++;}
                cb[ci]=0;
                aios_ui_input_clear();
                if(cb[0]){extern void ai_process_input(const char*);ai_process_input(cb);}
                aios_ui_prompt();
            } else terminal_handle_key('\n');
            continue;
        }
        if(sc==0x0F){
            terminal_handle_key(' ');terminal_handle_key(' ');
            terminal_handle_key(' ');terminal_handle_key(' ');
            continue;
        }
        if(sc==0x01){
            terminal_reset_input();
            terminal_render_prompt();
            if(vga_active){vga_shell_newline();vga_shell_prompt();}
            continue;
        }
        if(ctrl_held){
            if(sc==0x2E){
                terminal_print_color("^C\n",MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
                if(vga_active) vga_shell_print("^C\n",4);
                terminal_reset_input();
                terminal_render_prompt();
                if(vga_active) vga_shell_prompt();
                continue;
            }
            if(sc==0x26){
                terminal_clear();
                terminal_render_prompt();
                if(vga_active){aios_ui_init();}
                continue;
            }
            if(sc==0x2F){
                for(int i=0;i<clipboard_len;i++)
                    terminal_handle_key(clipboard[i]);
                continue;
            }
            if(sc==0x2E){
                terminal_copy_to_clipboard(clipboard,&clipboard_len);
                terminal_print_color("^C",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                continue;
            }
            continue;
        }
        if(sc<sizeof(sc2a)){
            char ch=shift_held?sc2a_shift[sc]:sc2a[sc];
            if(caps_lock&&ch>='a'&&ch<='z') ch-=32;
            else if(caps_lock&&ch>='A'&&ch<='Z') ch+=32;
            if(ch){if(aios_ui_active)aios_ui_input_char(ch);else terminal_handle_key(ch);}
        }
    }
}
