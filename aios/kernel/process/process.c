#include "process.h"
#include "../mm/heap.h"
#include "../mm/pmm.h"
#include "../terminal/terminal.h"

static process_t proc_table[MAX_PROCESSES];
static unsigned int next_pid = 1;

static void str_copy(unsigned char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max-1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static int str_len(const char* s) {
    int n = 0; while (*s++) n++; return n;
}

void process_init() {
    for (int i = 0; i < MAX_PROCESSES; i++)
        proc_table[i].state = PROC_EMPTY;
    terminal_print_color("Process manager  : OK\n",
                         MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
}

int process_spawn(const char* name, proc_entry_t entry) {
    // Find empty slot
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state == PROC_EMPTY) {
            proc_table[i].pid   = next_pid++;
            proc_table[i].state = PROC_READY;
            proc_table[i].entry = (unsigned int)entry;

            // Allocate stack
            void* stack = kmalloc(STACK_SIZE);
            proc_table[i].stack_top = (unsigned int)stack + STACK_SIZE;
            str_copy(proc_table[i].name, name, 32);

            terminal_print_color("[PROC] Spawned: ", MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
            terminal_print((char*)proc_table[i].name);
            terminal_print_color(" PID=", MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
            terminal_print_int(proc_table[i].pid);
            terminal_newline();

            // Run it directly (cooperative, no preemption yet)
            proc_table[i].state = PROC_RUNNING;
            entry();
            proc_table[i].state = PROC_DEAD;

            return proc_table[i].pid;
        }
    }
    return -1;
}

void process_exit(unsigned int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].pid == pid) {
            proc_table[i].state = PROC_DEAD;
            terminal_print_color("[PROC] Exited PID=",
                                 MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
            terminal_print_int(pid);
            terminal_newline();
            return;
        }
    }
}

void process_list() {
    terminal_print_color("PID  STATE    NAME\n",
                         MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
    terminal_print_color("---  -------  ----\n",
                         MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state != PROC_EMPTY) {
            terminal_print_int(proc_table[i].pid);
            terminal_print("    ");
            switch(proc_table[i].state) {
                case PROC_READY:   terminal_print_color("READY   ", MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK)); break;
                case PROC_RUNNING: terminal_print_color("RUNNING ", MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK)); break;
                case PROC_DEAD:    terminal_print_color("DEAD    ", MAKE_COLOR(COLOR_BRED,COLOR_BLACK)); break;
            }
            terminal_print("  ");
            terminal_print((char*)proc_table[i].name);
            terminal_newline();
            found++;
        }
    }
    if (!found) terminal_print("No processes\n");
}

// Load and execute a flat binary blob in memory
int process_exec_binary(const char* name, void* binary, unsigned int size) {
    // Allocate memory for the binary
    void* mem = kmalloc(size + STACK_SIZE);
    if (!mem) return -1;

    // Copy binary into allocated memory
    unsigned char* dst = (unsigned char*)mem;
    unsigned char* src = (unsigned char*)binary;
    for (unsigned int i = 0; i < size; i++) dst[i] = src[i];

    // Find empty process slot
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state == PROC_EMPTY) {
            proc_table[i].pid       = next_pid++;
            proc_table[i].state     = PROC_RUNNING;
            proc_table[i].entry     = (unsigned int)mem;
            proc_table[i].stack_top = (unsigned int)mem + size + STACK_SIZE;
            str_copy(proc_table[i].name, name, 32);

            terminal_print_color("[PROC] exec: ", MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            terminal_print((char*)proc_table[i].name);
            terminal_print_color(" @ ", MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            terminal_print_hex((unsigned int)mem);
            terminal_newline();

            // Jump to binary entry point
            proc_entry_t entry = (proc_entry_t)mem;
            entry();

            proc_table[i].state = PROC_DEAD;
            return proc_table[i].pid;
        }
    }
    return -1;
}
