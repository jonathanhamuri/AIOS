#ifndef SYSCALL_H
#define SYSCALL_H

// Syscall numbers — user programs put these in eax before int 0x80
#define SYS_EXIT     0
#define SYS_PRINT    1
#define SYS_MALLOC   2
#define SYS_FREE     3
#define SYS_INPUT    4
#define SYS_EXEC     5
#define SYS_MEMINFO  6
#define SYS_AI       7    // Send string to AI processor — the key syscall

#define SYSCALL_COUNT 8

// Return codes
#define SYS_OK       0
#define SYS_ERR     -1
#define SYS_NOMEM   -2
#define SYS_NOTFOUND -3

// Syscall handler type
typedef int (*syscall_fn_t)(unsigned int arg1,
                             unsigned int arg2,
                             unsigned int arg3);

void syscall_init();
int  syscall_dispatch(unsigned int num,
                      unsigned int arg1,
                      unsigned int arg2,
                      unsigned int arg3);

#endif
