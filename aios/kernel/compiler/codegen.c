#include "codegen.h"

static unsigned int dispatch_addr = 0;

void codegen_set_dispatch_addr(unsigned int addr) {
    dispatch_addr = addr;
}

void codegen_init(codegen_t* cg) {
    cg->pos       = 0;
    cg->var_count = 0;
    cg->var_frame = 0;
    cg->str_count = 0;
    cg->error     = 0;
    cg->errmsg[0] = 0;
}

void emit_byte(codegen_t* cg, unsigned char b) {
    if(cg->pos < CODE_BUF_SIZE) cg->buf[cg->pos++] = b;
}

void emit_dword(codegen_t* cg, unsigned int d) {
    emit_byte(cg, d & 0xFF);
    emit_byte(cg, (d>>8) & 0xFF);
    emit_byte(cg, (d>>16) & 0xFF);
    emit_byte(cg, (d>>24) & 0xFF);
}

void emit_bytes(codegen_t* cg, const unsigned char* b, int n) {
    for(int i=0;i<n;i++) emit_byte(cg, b[i]);
}

// Function prologue: push ebp / mov ebp,esp / sub esp,N
void emit_prologue(codegen_t* cg) {
    emit_byte(cg, 0x55);           // push ebp
    emit_byte(cg, 0x89); emit_byte(cg, 0xE5);  // mov ebp, esp
    // Reserve 256 bytes for locals
    emit_byte(cg, 0x81); emit_byte(cg, 0xEC);
    emit_dword(cg, 256);           // sub esp, 256
}

// Function epilogue: mov esp,ebp / pop ebp / ret
void emit_epilogue(codegen_t* cg) {
    emit_byte(cg, 0x89); emit_byte(cg, 0xEC);  // mov esp, ebp
    emit_byte(cg, 0x5D);           // pop ebp
    emit_byte(cg, 0xC3);           // ret
}

// push imm32
void emit_push_imm(codegen_t* cg, unsigned int val) {
    emit_byte(cg, 0x68);
    emit_dword(cg, val);
}

// Store string in code buffer, push its address
void emit_push_str(codegen_t* cg, const char* s) {
    // We'll append the string after the code
    // For now store it inline and use its address
    // The string goes at current pos+6 (after the jmp over it)
    // jmp over string
    int slen = 0;
    while(s[slen]) slen++;
    slen++; // null terminator

    // emit: jmp +slen (short jump)
    emit_byte(cg, 0xEB);
    emit_byte(cg, (unsigned char)slen);

    // Remember string address
    unsigned int str_addr = (unsigned int)cg->buf + cg->pos;

    // Emit string bytes
    for(int i=0;i<slen;i++) emit_byte(cg, (unsigned char)s[i]);

    // push str_addr
    emit_byte(cg, 0x68);
    emit_dword(cg, str_addr);
}

// Call syscall_dispatch(num, arg1, arg2, arg3)
// Args already on stack in reverse order before this call
// We do: push 0 / push 0 / push arg1 / push num / call dispatch / add esp,16
void emit_call_syscall(codegen_t* cg, int syscall_num) {
    if(!dispatch_addr) return;

    // Args: num, arg1, arg2=0, arg3=0
    // Stack already has arg1 pushed
    // push 0 (arg3)
    emit_push_imm(cg, 0);
    // push 0 (arg2 = color default)
    emit_push_imm(cg, 0x0F00); // white on black
    // syscall num already needs to go first
    // Actually reorder: call dispatch(num, str_ptr, color, 0)
    // For print: stack = [str_ptr] -> we need [num, str_ptr, color, 0]
    // Use registers instead
    // pop eax (str_ptr) 
    emit_byte(cg, 0x58);       // pop eax  -> str_ptr
    // Fix: just push all 4 args
    emit_push_imm(cg, 0);      // arg3
    emit_push_imm(cg, 0x0F00); // arg2 color
    emit_byte(cg, 0x50);       // push eax  -> arg1 = str_ptr
    emit_push_imm(cg, syscall_num); // arg0 = syscall num

    // call dispatch_addr
    emit_byte(cg, 0xE8);
    // relative offset = dispatch_addr - (current_addr + 4)
    unsigned int cur = (unsigned int)cg->buf + cg->pos + 4;
    emit_dword(cg, dispatch_addr - cur);

    // add esp, 16
    emit_byte(cg, 0x83); emit_byte(cg, 0xC4); emit_byte(cg, 0x10);
}

void emit_ret(codegen_t* cg) {
    emit_byte(cg, 0xC3);
}
