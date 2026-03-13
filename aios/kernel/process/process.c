#include "process.h"
#include "../mm/heap.h"
#include "../terminal/terminal.h"

process_t proc_table[MAX_PROCESSES];
int current_pid = -1;
volatile unsigned int timer_ticks = 0;

static void scopy(char* d, const char* s, int m) {
    int i=0; while(s[i]&&i<m-1){d[i]=s[i];i++;} d[i]=0;
}

void process_init() {
    for(int i=0;i<MAX_PROCESSES;i++){
        proc_table[i].state=PROC_EMPTY;
        proc_table[i].pid=0;
    }
    terminal_print_color("Process manager  : OK\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

int process_spawn(const char* name, proc_entry_t entry) {
    for(int i=0;i<MAX_PROCESSES;i++){
        if(proc_table[i].state==PROC_EMPTY){
            proc_table[i].pid=i+1;
            proc_table[i].state=PROC_READY;
            proc_table[i].entry=(unsigned int)entry;
            scopy((char*)proc_table[i].name,name,32);
            unsigned char* stack=(unsigned char*)kmalloc(STACK_SIZE);
            proc_table[i].stack_top=(unsigned int)(stack+STACK_SIZE);
            unsigned int* sp=(unsigned int*)(proc_table[i].stack_top);
            sp--; *sp=0;
            sp--; *sp=(unsigned int)entry;
            sp--; *sp=0; sp--; *sp=0; sp--; *sp=0; sp--; *sp=0;
            proc_table[i].esp=(unsigned int)sp;
            proc_table[i].eip=(unsigned int)entry;
            return i+1;
        }
    }
    return -1;
}

void process_exit(unsigned int pid) {
    for(int i=0;i<MAX_PROCESSES;i++)
        if(proc_table[i].pid==pid) proc_table[i].state=PROC_DEAD;
}

void process_list() {
    terminal_print_color("PID  STATE    NAME\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    for(int i=0;i<MAX_PROCESSES;i++){
        if(proc_table[i].state!=PROC_EMPTY){
            terminal_print_int(proc_table[i].pid);
            terminal_print("    ");
            const char* st="?      ";
            if(proc_table[i].state==PROC_READY)   st="READY  ";
            if(proc_table[i].state==PROC_RUNNING)  st="RUNNING";
            if(proc_table[i].state==PROC_DEAD)     st="DEAD   ";
            terminal_print(st);
            terminal_print("  ");
            terminal_print((const char*)proc_table[i].name);
            terminal_newline();
        }
    }
}

int process_exec_binary(const char* name, void* binary, unsigned int size) {
    void* mem=kmalloc(size);
    if(!mem) return -1;
    unsigned char* src=(unsigned char*)binary;
    unsigned char* dst=(unsigned char*)mem;
    for(unsigned int i=0;i<size;i++) dst[i]=src[i];
    terminal_print_color("[PROC] exec: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print(name);
    terminal_print_color(" @ 0x",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print_hex((unsigned int)mem);
    terminal_newline();
    typedef void (*fn_t)();
    ((fn_t)mem)();
    kfree(mem);
    return 0;
}

void scheduler_tick() {
    // Find and run next ready process
    for(int i=0;i<MAX_PROCESSES;i++){
        if(proc_table[i].state==PROC_READY){
            proc_table[i].state=PROC_RUNNING;
            current_pid=i;
            typedef void (*fn_t)();
            fn_t fn=(fn_t)proc_table[i].entry;
            if(fn && (unsigned int)fn > 0x100000) fn();
            proc_table[i].state=PROC_DEAD;
            current_pid=-1;
            return;
        }
    }
    // original:

    int next=-1;
    for(int i=0;i<MAX_PROCESSES;i++){
        int idx=(current_pid+1+i)%MAX_PROCESSES;
        if(proc_table[idx].state==PROC_READY){next=idx;break;}
    }
    if(next==-1) return;
    if(current_pid>=0&&proc_table[current_pid].state==PROC_RUNNING)
        proc_table[current_pid].state=PROC_READY;
    current_pid=next;
    proc_table[current_pid].state=PROC_RUNNING;

    // Execute the process entry point (cooperative for now)
    terminal_print_color("[SCHED] entry=0x",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_hex(proc_table[current_pid].entry);
    terminal_newline();
    typedef void (*fn_t)();
    fn_t fn = (fn_t)proc_table[current_pid].entry;
    if(fn) {
        fn();
        // Mark done after one execution
        proc_table[current_pid].state=PROC_DEAD;
        current_pid=-1;
    }
}

int scheduler_current_pid() { return current_pid; }
void scheduler_yield() { scheduler_tick(); }
