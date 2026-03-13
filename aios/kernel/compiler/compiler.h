#ifndef COMPILER_H
#define COMPILER_H

#define COMPILE_OK    0
#define COMPILE_ERR  -1

typedef struct {
    unsigned char* code;
    unsigned int   size;
    char           errmsg[256];
} compile_result_t;

void compiler_init(unsigned int syscall_dispatch_addr);
int  compiler_compile(const char* src, compile_result_t* result);

#endif
