#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES  16
#define STACK_SIZE     0x4000   // 16KB stack per process

#define PROC_EMPTY     0
#define PROC_READY     1
#define PROC_RUNNING   2
#define PROC_DEAD      3

typedef void (*proc_entry_t)();

typedef struct {
    unsigned int  pid;
    unsigned int  state;
    unsigned int  entry;        // entry point address
    unsigned int  stack_top;    // top of stack
    unsigned char name[32];
} process_t;

void  process_init();
int   process_spawn(const char* name, proc_entry_t entry);
void  process_exit(unsigned int pid);
void  process_list();
int   process_exec_binary(const char* name, void* binary, unsigned int size);

#endif
