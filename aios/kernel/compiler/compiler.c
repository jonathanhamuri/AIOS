#include "compiler.h"
#include "lexer.h"
#include "codegen.h"
#include "../mm/heap.h"
#include "../terminal/terminal.h"
#include "../syscall/syscall.h"

static lexer_t   lexer;
static codegen_t cg;

// Current token index
static int tok_pos;
static token_t* toks;

static token_t* peek()    { return &toks[tok_pos]; }
static token_t* advance() { return &toks[tok_pos++]; }
static int check(token_type_t t) { return toks[tok_pos].type == t; }
static int match(token_type_t t) {
    if(check(t)) { tok_pos++; return 1; }
    return 0;
}

static int str_len(const char* s) { int n=0; while(*s++)n++; return n; }
static void str_copy(char* d, const char* s) { while(*s)*d++=*s++; *d=0; }

// Forward declarations
static int parse_stmt();
static int parse_expr();

// print "string";  or  print expr;
static int parse_print() {
    if(check(TOK_STRING)) {
        token_t* t = advance();
        // push string, call SYS_PRINT
        emit_push_str(&cg, t->value);
        // pop eax, push args, call
        emit_byte(&cg, 0x58);       // pop eax (str addr)
        emit_push_imm(&cg, 0);      // arg3
        emit_push_imm(&cg, 0x0A00); // arg2 = green color
        emit_byte(&cg, 0x50);       // push eax = arg1
        emit_push_imm(&cg, SYS_PRINT); // arg0
        unsigned int dispatch = 0;
        // get dispatch addr via extern - we stored it
        extern unsigned int _compiler_dispatch_addr;
        unsigned int cur = (unsigned int)cg.buf + cg.pos + 4 + 1;
        emit_byte(&cg, 0xE8);
        emit_dword(&cg, _compiler_dispatch_addr - ((unsigned int)cg.buf + cg.pos + 4));
        emit_byte(&cg, 0x83); emit_byte(&cg, 0xC4); emit_byte(&cg, 0x10); // add esp,16
    }
    match(TOK_SEMI);
    return 1;
}

// ai "string";  - sends to AI processor
static int parse_ai() {
    if(check(TOK_STRING)) {
        token_t* t = advance();
        emit_push_str(&cg, t->value);
        emit_byte(&cg, 0x58);
        emit_push_imm(&cg, 0);
        emit_push_imm(&cg, 0);
        emit_byte(&cg, 0x50);
        emit_push_imm(&cg, SYS_AI);
        extern unsigned int _compiler_dispatch_addr;
        emit_byte(&cg, 0xE8);
        emit_dword(&cg, _compiler_dispatch_addr - ((unsigned int)cg.buf + cg.pos + 4));
        emit_byte(&cg, 0x83); emit_byte(&cg, 0xC4); emit_byte(&cg, 0x10);
    }
    match(TOK_SEMI);
    return 1;
}

static int parse_stmt() {
    token_t* t = peek();
    if(t->type == TOK_EOF) return 0;

    if(match(TOK_PRINT)) return parse_print();
    if(match(TOK_AI))    return parse_ai();

    // Unknown - skip token
    advance();
    return 1;
}

unsigned int _compiler_dispatch_addr = 0;

void compiler_init(unsigned int dispatch_addr) {
    _compiler_dispatch_addr = dispatch_addr;
}

int compiler_compile(const char* src, compile_result_t* result) {
    // Lex
    lexer_init(&lexer, src);
    lexer_tokenize(&lexer);

    if(lexer.error) {
        str_copy(result->errmsg, lexer.errmsg);
        return COMPILE_ERR;
    }

    // Init codegen
    codegen_init(&cg);
    tok_pos = 0;
    toks = lexer.tokens;

    // Emit function prologue
    emit_prologue(&cg);

    // Parse and emit all statements
    while(!check(TOK_EOF)) {
        if(!parse_stmt()) break;
    }

    // Emit epilogue
    emit_epilogue(&cg);

    // Allocate output buffer
    unsigned char* out = (unsigned char*)kmalloc(cg.pos);
    if(!out) {
        str_copy(result->errmsg, "out of memory");
        return COMPILE_ERR;
    }

    // Copy code
    for(int i=0; i<cg.pos; i++) out[i] = cg.buf[i];

    result->code = out;
    result->size = cg.pos;
    result->errmsg[0] = 0;

    return COMPILE_OK;
}
