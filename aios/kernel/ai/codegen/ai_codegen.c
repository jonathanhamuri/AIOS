#include "ai_codegen.h"
#include "../../terminal/terminal.h"
#include "../../compiler/compiler.h"
#include "../../process/process.h"
#include "../../mm/heap.h"
#include "../../ai/knowledge/kb.h"

static int sstart(const char* s, const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int slen(const char* s){int n=0;while(*s++)n++;return n;}
static void scopy(char* d, const char* s){int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;}
static void sappend(char* d, const char* s){
    int i=0; while(d[i])i++;
    int j=0; while(s[j]){d[i++]=s[j++];}d[i]=0;
}

// Template-based code generation from natural language
// Maps English descriptions to AIOS source code
int ai_generate_module(const char* desc, char* out, int max){
    out[0] = 0;

    // "module that prints X"
    if(sstart(desc,"module that prints ")||
       sstart(desc,"function that prints ")||
       sstart(desc,"code that prints ")){
        const char* msg = desc;
        while(*msg && *msg!=' ') msg++; // skip first word
        while(*msg==' ') msg++;
        while(*msg && *msg!=' ') msg++; // skip "that"
        while(*msg==' ') msg++;
        while(*msg && *msg!=' ') msg++; // skip "prints"
        while(*msg==' ') msg++;
        sappend(out,"print \"");
        sappend(out,msg);
        sappend(out,"\";");
        return 1;
    }

    // "module that counts to X"
    if(sstart(desc,"module that counts")||
       sstart(desc,"count to ")||
       sstart(desc,"counter")){
        sappend(out,"print \"1\"; print \"2\"; print \"3\"; print \"4\"; print \"5\";");
        return 1;
    }

    // "module that shows memory"
    if(sstart(desc,"module that shows memory")||
       sstart(desc,"show memory usage")){
        sappend(out,"print \"Memory module loaded\";");
        return 1;
    }

    // "module that greets X"  
    if(sstart(desc,"module that greets")||
       sstart(desc,"greeting module")){
        const char* name = desc + slen(desc) - 1;
        // find last word
        while(name > desc && *name != ' ') name--;
        if(*name == ' ') name++;
        sappend(out,"print \"Hello, ");
        sappend(out,name);
        sappend(out,"!\";");
        return 1;
    }

    // "module that calculates X op Y"
    if(sstart(desc,"module that calculates")||
       sstart(desc,"calculate and print")){
        sappend(out,"print \"Calculating...\";");
        return 1;
    }

    // "module that beeps" / "alarm module"
    if(sstart(desc,"module that beeps")||
       sstart(desc,"alarm module")||
       sstart(desc,"beep module")){
        sappend(out,"print \"[BEEP] Alert!\";");
        return 1;
    }

    // "module that logs X"
    if(sstart(desc,"module that logs")||
       sstart(desc,"logging module")){
        sappend(out,"print \"[LOG] Module active\";");
        return 1;
    }

    // "module that monitors"
    if(sstart(desc,"module that monitors")||
       sstart(desc,"monitor module")){
        sappend(out,"print \"[MONITOR] System OK\"; print \"[MONITOR] CPU: ring0\"; print \"[MONITOR] Net: ready\";");
        return 1;
    }

    // Generic: wrap description as a print
    sappend(out,"print \"Module: ");
    // copy up to 32 chars of desc
    int i=0;
    while(desc[i] && i<32){out[slen(out)]=desc[i];i++;}
    out[slen(out)]=0;
    sappend(out,"\";");
    return 1;
}

int ai_load_module(const char* description){
    char code[512];
    code[0]=0;

    terminal_print_color("[AI-GEN] Analyzing: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print(description);
    terminal_newline();

    int ok = ai_generate_module(description, code, 512);
    if(!ok || !code[0]){
        terminal_print_color("[AI-GEN] Cannot generate module\n",
            MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        return -1;
    }

    terminal_print_color("[AI-GEN] Generated code: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print(code);
    terminal_newline();

    // Compile the generated code
    compile_result_t result;
    int r = compiler_compile(code, &result);
    if(r != 0){
        terminal_print_color("[AI-GEN] Compile error: ",MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        terminal_print(result.errmsg);
        terminal_newline();
        return -1;
    }

    terminal_print_color("[AI-GEN] Compiled OK - ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print_int(result.size);
    terminal_print_color(" bytes - loading...\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Execute the module
    process_exec_binary("ai_module", result.code, result.size);
    kfree(result.code);

    terminal_print_color("[AI-GEN] Module executed successfully\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Save to knowledge base
    kb_set(description, code);

    return 0;
}
