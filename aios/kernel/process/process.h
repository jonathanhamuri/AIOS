#ifndef PROCESS_H
#define PROCESS_H
#define MAX_PROCESSES  16
#define STACK_SIZE     0x4000
#define PROC_EMPTY     0
#define PROC_READY     1
#define PROC_RUNNING   2
#define PROC_DEAD      3
typedef void (*proc_entry_t)();
typedef struct {
    unsigned int pid;
    unsigned int state;
    unsigned int entry;
    unsigned int stack_top;
    unsigned int esp;
    unsigned int eip;
    unsigned int ebx,esi,edi,ebp;
    unsigned char name[32];
} process_t;
void process_init();
int  process_spawn(const char* name, proc_entry_t entry);
void process_exit(unsigned int pid);
void process_list();
int  process_exec_binary(const char* name, void* binary, unsigned int size);
void scheduler_tick();
int  scheduler_current_pid();
void scheduler_yield();
extern process_t proc_table[MAX_PROCESSES];
extern int current_pid;
extern unsigned int timer_ticks_bss;
#endif
