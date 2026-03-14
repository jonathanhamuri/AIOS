#include "kb.h"
#include "../../terminal/terminal.h"
#include "../../mm/heap.h"

static kb_fact_t    facts[KB_MAX_FACTS];
static kb_command_t commands[KB_MAX_COMMANDS];
static int facts_init_done = 0;

// ── string helpers ────────────────────────────────────────────────────
static int seq(const char*a,const char*b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int sstart(const char*s,const char*p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static void scopy(char*d,const char*s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}
static int slen(const char*s){int n=0;while(*s++)n++;return n;}

void kb_init() {
    for(int i=0;i<KB_MAX_FACTS;i++)    facts[i].active=0;
    for(int i=0;i<KB_MAX_COMMANDS;i++) commands[i].active=0;
    facts_init_done=1;

    // Seed AIOS self-knowledge
    kb_set("name",        "AIOS");
    kb_set("version",     "0.5.0-phase5");
    kb_set("arch",        "x86 32-bit protected mode");
    kb_set("language",    "Assembly + C + AIOS native");
    kb_set("memory_mgr",  "bitmap PMM + free-list heap");
    kb_set("syscalls",    "8 (exit,print,malloc,free,input,exec,meminfo,ai)");
    kb_set("compiler",    "AIOS native - lexer+codegen+x86 emitter");
    kb_set("ai_engine",   "intent parser EN/FR phase4");
    kb_set("phases_done", "1,2,3,4");
    kb_set("phase_current","5");
    kb_set("creator",     "built from scratch - no borrowed OS code");
    kb_set("purpose",     "AI-native operating system");

    terminal_print_color("Knowledge base   : OK\n",
                         MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void kb_set(const char* key, const char* val) {
    // Update if exists
    for(int i=0;i<KB_MAX_FACTS;i++){
        if(facts[i].active && seq(facts[i].key,key)){
            scopy(facts[i].value,val,KB_VAL_LEN);
            return;
        }
    }
    // Insert new
    for(int i=0;i<KB_MAX_FACTS;i++){
        if(!facts[i].active){
            facts[i].active=1;
            scopy(facts[i].key,key,KB_KEY_LEN);
            scopy(facts[i].value,val,KB_VAL_LEN);
            return;
        }
    }
}

const char* kb_get(const char* key) {
    for(int i=0;i<KB_MAX_FACTS;i++)
        if(facts[i].active && seq(facts[i].key,key))
            return facts[i].value;
    return 0;
}

void kb_delete(const char* key) {
    for(int i=0;i<KB_MAX_FACTS;i++)
        if(facts[i].active && seq(facts[i].key,key))
            facts[i].active=0;
}

void kb_list() {
    terminal_print_color("AIOS Knowledge Base:\n",
                         MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    int count=0;
    for(int i=0;i<KB_MAX_FACTS;i++){
        if(facts[i].active){
            terminal_print_color("  ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(facts[i].key,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
            terminal_print_color(" = ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(facts[i].value,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            terminal_newline();
            count++;
        }
    }
    terminal_print_color("Total: ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print_int(count);
    terminal_print_color(" facts\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
}

int kb_learn(const char* trigger, const char* source) {
    // Update if exists
    for(int i=0;i<KB_MAX_COMMANDS;i++){
        if(commands[i].active && seq(commands[i].trigger,trigger)){
            scopy(commands[i].source,source,KB_VAL_LEN);
            return 1;
        }
    }
    // Insert new
    for(int i=0;i<KB_MAX_COMMANDS;i++){
        if(!commands[i].active){
            commands[i].active=1;
            scopy(commands[i].trigger,trigger,KB_KEY_LEN);
            scopy(commands[i].source,source,KB_VAL_LEN);
            return 1;
        }
    }
    return 0;
}

int kb_recall(const char* input) {
    for(int i=0;i<KB_MAX_COMMANDS;i++){
        if(commands[i].active && sstart(input,commands[i].trigger)){
            return i;  // return index
        }
    }
    return -1;
}

void kb_list_commands() {
    terminal_print_color("Learned commands:\n",
                         MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    int count=0;
    for(int i=0;i<KB_MAX_COMMANDS;i++){
        if(commands[i].active){
            terminal_print_color("  ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(commands[i].trigger,
                                 MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
            terminal_print_color(" -> ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(commands[i].source,
                                 MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            terminal_newline();
            count++;
        }
    }
    if(!count) terminal_print("  No learned commands yet\n");
}

const char* commands_get_source(int idx) {
    if(idx>=0 && idx<KB_MAX_COMMANDS && commands[idx].active)
        return commands[idx].source;
    return "";
}
const char* commands_get_trigger(int idx) {
    if(idx>=0 && idx<KB_MAX_COMMANDS && commands[idx].active)
        return commands[idx].trigger;
    return "";
}

const char* kb_get_key(int idx) {
    if(idx>=0&&idx<KB_MAX_FACTS&&facts[idx].active) return facts[idx].key;
    return 0;
}
const char* kb_get_val(int idx) {
    if(idx>=0&&idx<KB_MAX_FACTS&&facts[idx].active) return facts[idx].value;
    return 0;
}
int kb_get_active(int idx) {
    if(idx>=0&&idx<KB_MAX_FACTS) return facts[idx].active;
    return 0;
}
int commands_get_active(int idx) {
    if(idx>=0&&idx<KB_MAX_COMMANDS) return commands[idx].active;
    return 0;
}
