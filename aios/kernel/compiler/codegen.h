#ifndef CODEGEN_H
#define CODEGEN_H

#define CODE_BUF_SIZE  65536   // 64KB max compiled binary
#define MAX_VARS       64
#define MAX_STRINGS    64
#define MAX_STR_LEN    256

typedef struct {
    char name[32];
    int  offset;    // offset from ebp
} var_t;

typedef struct {
    char data[MAX_STR_LEN];
    int  code_offset;   // where in code buffer this string is stored
} str_entry_t;

typedef struct {
    unsigned char buf[CODE_BUF_SIZE];
    int           pos;          // current write position
    var_t         vars[MAX_VARS];
    int           var_count;
    int           var_frame;    // total frame size
    str_entry_t   strings[MAX_STRINGS];
    int           str_count;
    int           error;
    char          errmsg[128];
} codegen_t;

void codegen_init(codegen_t* cg);

// Emit raw bytes
void emit_byte(codegen_t* cg, unsigned char b);
void emit_dword(codegen_t* cg, unsigned int d);
void emit_bytes(codegen_t* cg, const unsigned char* b, int n);

// High level emitters
void emit_prologue(codegen_t* cg);
void emit_epilogue(codegen_t* cg);
void emit_push_imm(codegen_t* cg, unsigned int val);
void emit_push_str(codegen_t* cg, const char* s);
void emit_call_syscall(codegen_t* cg, int syscall_num);
void emit_ret(codegen_t* cg);

// Address of kernel syscall_dispatch for direct call
void codegen_set_dispatch_addr(unsigned int addr);

#endif
