#include "self_extend.h"
#include "../../process/process.h"
#include "kb.h"
#include "disk/kbfs.h"
#include "disk/ata.h"
#include "../../terminal/terminal.h"
#include "../../compiler/compiler.h"
#include "../../process/process.h"
#include "../../mm/heap.h"

static int seq(const char*a,const char*b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int sstart(const char*s,const char*p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static void scopy(char*d,const char*s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}
static const char* skip_word(const char*s){
    while(*s&&*s!=' ')s++;while(*s==' ')s++;return s;
}
static const char* skip_words(const char*s,int n){
    for(int i=0;i<n;i++)s=skip_word(s);return s;
}

// Parse: "learn <trigger> = <source>"
// Example: learn greet_user = print "Welcome to AIOS!";
static int parse_learn(const char* input) {
    // input starts after "learn "
    const char* p = input;

    // Find trigger (up to =)
    char trigger[64]; int ti=0;
    while(*p && *p!='=' && ti<63){
        if(*p!=' '||ti>0) trigger[ti++]=*p;
        p++;
    }
    // Trim trailing spaces from trigger
    while(ti>0&&trigger[ti-1]==' ')ti--;
    trigger[ti]=0;

    if(*p!='='){
        terminal_print_color("Usage: learn <trigger> = <aios source>\n",
                             MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        return 0;
    }
    p++; // skip =
    while(*p==' ')p++; // skip spaces

    const char* source = p;

    if(!trigger[0]||!source[0]){
        terminal_print_color("Error: empty trigger or source\n",
                             MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        return 0;
    }

    // Store in knowledge base
    kb_learn(trigger, source);

    terminal_print_color("[AIOS] Learned: '",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print(trigger);
    terminal_print_color("' -> '",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print(source);
    terminal_print_color("'\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print_color("[AIOS] New command registered. Try it now!\n",
                         MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    return 1;
}

// Parse: "remember <key> = <value>"
static int parse_remember(const char* input) {
    char key[64]; int ki=0;
    const char* p=input;
    while(*p&&*p!='='&&ki<63){
        if(*p!=' '||ki>0)key[ki++]=*p;
        p++;
    }
    while(ki>0&&key[ki-1]==' ')ki--;
    key[ki]=0;
    if(*p=='=')p++;
    while(*p==' ')p++;

    kb_set(key,p);
    terminal_print_color("[AIOS] Remembered: ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print(key);
    terminal_print_color(" = ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print(p);
    terminal_newline();
    return 1;
}

// Parse: "what is <key>" / "what do you know about <key>"
static int parse_query(const char* input) {
    // Try to find a matching key in kb
    const char* val = kb_get(input);
    if(val){
        terminal_print_color(input,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_color(" = ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_color(val,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        terminal_newline();
        return 1;
    }
    return 0;
}

void self_extend_init() {
    terminal_print_color("Self-extension   : OK\n",
                         MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

int self_extend_parse(const char* input) {
    if(sstart(input,"spawn ")||sstart(input,"run background")){
        const char* task_name = input+6;
        int pid = process_spawn(task_name, (proc_entry_t)0);
        if(pid>0){
            terminal_print_color("[AIOS] Spawned PID ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
            terminal_print_int(pid);
            terminal_newline();
        }
        return 1;
    }
    if(sstart(input,"ps")||sstart(input,"processes")||sstart(input,"list proc")){
        process_list();
        return 1;
    }
    if(sstart(input,"ticks")||sstart(input,"timer")){
        extern unsigned int timer_ticks_bss;
        terminal_print_color("[TIMER] ticks=",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print_int(timer_ticks_bss);
        terminal_newline();
        return 1;
    }
    if(sstart(input,"ps")||sstart(input,"processes")||sstart(input,"list proc")){
        process_list();
        return 1;
    }
    if(sstart(input,"save")||sstart(input,"sauvegarde")){
        kbfs_save();
        return 1;
    }
    if(seq(input,"load")||sstart(input,"load knowledge")){
        kbfs_load();
        return 1;
    }
    if(sstart(input,"remember ")){
        return parse_remember(input+9);
    }
    if(sstart(input,"forget ")){
        kb_delete(input+7);
        terminal_print_color("[AIOS] Forgotten: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print(input+7);
        terminal_newline();
        return 1;
    }
    if(seq(input,"knowledge")||seq(input,"kb")||sstart(input,"what do you know")){
        kb_list();
        return 1;
    }
    if(seq(input,"commands")||sstart(input,"learned commands")){
        kb_list_commands();
        return 1;
    }

    // Try learned command recall
    int idx = kb_recall(input);
    if(idx>=0){
        // Compile and run the stored source
        compile_result_t result;
        char src[256];
        scopy(src, commands_get_source(idx), 256);

        terminal_print_color("[AIOS] Running learned command...\n",
                             MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        int r = compiler_compile(src, &result);
        if(r==0){
            process_exec_binary("learned", result.code, result.size);
            kfree(result.code);
        }
        return 1;
    }

    return 0;
}
