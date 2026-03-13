#include "syscall.h"
#include "../terminal/terminal.h"
#include "../terminal/ai_input.h"
#include "../mm/heap.h"
#include "../mm/pmm.h"

// ── individual syscall handlers ───────────────────────────────────────

static int sys_exit_handler(unsigned int code,
                             unsigned int unused1,
                             unsigned int unused2) {
    terminal_print_color("\n[SYSCALL] Process exited with code ", 
                         MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
    terminal_print_int((int)code);
    terminal_newline();
    terminal_render_prompt();
    return SYS_OK;
}

static int sys_print_handler(unsigned int str_ptr,
                              unsigned int color,
                              unsigned int unused) {
    const char* s = (const char*)str_ptr;
    if (!s) return SYS_ERR;
    if (color == 0) color = MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK);
    terminal_print_color(s, (unsigned char)color);
    return SYS_OK;
}

static int sys_malloc_handler(unsigned int size,
                               unsigned int unused1,
                               unsigned int unused2) {
    void* ptr = kmalloc(size);
    return (int)ptr;
}

static int sys_free_handler(unsigned int ptr,
                             unsigned int unused1,
                             unsigned int unused2) {
    kfree((void*)ptr);
    return SYS_OK;
}

static int sys_input_handler(unsigned int buf_ptr,
                              unsigned int max_len,
                              unsigned int unused) {
    // Returns the current command buffer contents
    // In Phase 4 this will block until input is ready
    char* buf = (char*)buf_ptr;
    if (!buf) return SYS_ERR;
    buf[0] = 0;
    return SYS_OK;
}

static int sys_exec_handler(unsigned int entry_ptr,
                             unsigned int arg_ptr,
                             unsigned int unused) {
    // Call a function as if it were a program
    // Phase 3 will expand this into a full process loader
    typedef void (*entry_fn_t)(const char*);
    entry_fn_t fn = (entry_fn_t)entry_ptr;
    if (!fn) return SYS_ERR;
    fn((const char*)arg_ptr);
    return SYS_OK;
}

static int sys_meminfo_handler(unsigned int unused1,
                                unsigned int unused2,
                                unsigned int unused3) {
    terminal_print_color("Free pages : ", MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
    terminal_print_int(pmm_free_pages());
    terminal_print_color(" x 4KB = ", MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print_int(pmm_free_pages() * 4);
    terminal_print_color(" KB\n", MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    return SYS_OK;
}

static int sys_ai_handler(unsigned int str_ptr,
                           unsigned int unused1,
                           unsigned int unused2) {
    // THE core syscall — sends a string to the AI processor
    // This is what the AI language compiler will emit for every statement
    const char* input = (const char*)str_ptr;
    if (!input) return SYS_ERR;
    ai_process_input(input);
    return SYS_OK;
}

// ── dispatch table ────────────────────────────────────────────────────

static syscall_fn_t syscall_table[SYSCALL_COUNT] = {
    sys_exit_handler,    // 0 - SYS_EXIT
    sys_print_handler,   // 1 - SYS_PRINT
    sys_malloc_handler,  // 2 - SYS_MALLOC
    sys_free_handler,    // 3 - SYS_FREE
    sys_input_handler,   // 4 - SYS_INPUT
    sys_exec_handler,    // 5 - SYS_EXEC
    sys_meminfo_handler, // 6 - SYS_MEMINFO
    sys_ai_handler,      // 7 - SYS_AI
};

void syscall_init() {
    terminal_print_color("Syscall interface : OK (", 
                         MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
    terminal_print_int(SYSCALL_COUNT);
    terminal_print_color(" syscalls registered)\n",
                         MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
}

int syscall_dispatch(unsigned int num,
                     unsigned int arg1,
                     unsigned int arg2,
                     unsigned int arg3) {
    if (num >= SYSCALL_COUNT) return SYS_ERR;
    return syscall_table[num](arg1, arg2, arg3);
}
