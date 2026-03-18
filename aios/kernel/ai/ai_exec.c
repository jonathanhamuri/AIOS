#include "autonomy/autonomy.h"
#include "../apps/apps.h"
#include "documents/docgen.h"
#include "../net/discovery/discovery.h"
#include "engineering/engineering.h"
#include "learning/learning.h"
#include "codegen/ai_codegen.h"
#include "../graphics/vga_commands.h"
#include "ai_exec.h"
#include "intent.h"
#include "knowledge/kb.h"
#include "knowledge/self_extend.h"
#include "../terminal/terminal.h"
#include "../compiler/compiler.h"
#include "../compiler/lexer.h"
#include "../process/process.h"
#include "../mm/heap.h"
#include "../syscall/syscall.h"

static int  seq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int  sstart(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}
static int  slen(const char*s){int n=0;while(*s++)n++;return n;}

static void compile_and_run(const char* src){
    compile_result_t result;
    int r = compiler_compile(src, &result);
    if(r != 0){
        terminal_print_color("[AI] Compile error: ", MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        terminal_print(result.errmsg);
        terminal_newline();
        return;
    }
    process_exec_binary("ai_prog", result.code, result.size);
    kfree(result.code);
}

static int eval_expr(const char* s){
    int a=0,b=0;
    char op=0;
    int i=0;
    while(s[i]==' ')i++;
    while(s[i]>='0'&&s[i]<='9'){a=a*10+(s[i]-'0');i++;}
    while(s[i]==' ')i++;
    op=s[i];i++;
    while(s[i]==' ')i++;
    while(s[i]>='0'&&s[i]<='9'){b=b*10+(s[i]-'0');i++;}
    switch(op){
        case '+': return a+b;
        case '=': return a+b;
        case '-': return a-b;
        case '*': return a*b;
        case 'x': return a*b;
        case '/': return b?a/b:0;
        default:  return 0;
    }
}

void ai_exec_init(){
    terminal_print_color("AI language      : OK\n",
                         MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void ai_exec(const char* input){
    if(vga_handle_command(input)) return;
    if(self_extend_parse(input)) return;
    if(learning_handle(input)) return;
    if(engineering_handle(input)) return;
    if(discovery_handle(input)) return;
    if(docgen_handle(input)) return;
    if(apps_handle(input)) return;
    if(autonomy_handle(input)) return;
    autonomy_observe(input);

    // Phase 13: AI self-modification - generate and load modules
    if(sstart(input,"create module ")||
       sstart(input,"generate module ")||
       sstart(input,"build module ")||
       sstart(input,"make module ")||
       sstart(input,"write module ")||
       sstart(input,"module that ")||
       sstart(input,"load module ")){
        const char* desc = input;
        // skip first word if it's create/generate/build/make/write/load
        if(sstart(input,"create module ")) desc=input+14;
        else if(sstart(input,"generate module ")) desc=input+16;
        else if(sstart(input,"build module ")) desc=input+13;
        else if(sstart(input,"make module ")) desc=input+12;
        else if(sstart(input,"write module ")) desc=input+13;
        else if(sstart(input,"load module ")) desc=input+12;
        ai_load_module(desc);
        return;
    }

    intent_t intent;
    intent_parse(input, &intent);

    terminal_print_color("[AI:", MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print_color(intent_name(intent.type), MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print_color("] ", MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));

    switch(intent.type){

        case INTENT_GREET:
            if(seq(intent.lang,"fr")){
                terminal_print_color("Bonjour! AIOS ecoute.\n",
                                     MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color("Je comprends le francais et l'anglais.\n",
                                     MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            } else {
                terminal_print_color("Hello! AIOS is listening.\n",
                                     MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color("I understand English and French.\n",
                                     MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            }
            break;

        case INTENT_PRINT: {
            char src[256];
            char* p = src;
            const char* prefix = "print \"";
            while(*prefix)*p++=*prefix++;
            const char* a = intent.arg1;
            while(*a)*p++=*a++;
            *p++='"'; *p++=';'; *p++=0;
            compile_and_run(src);
            terminal_newline();
            break;
        }

        case INTENT_CLEAR:
            terminal_clear();
            terminal_render_prompt();
            return;

        case INTENT_MEMORY:
            syscall_dispatch(SYS_MEMINFO,0,0,0);
            break;

        case INTENT_HELP:
            if(seq(intent.lang,"fr")){
                terminal_print_color("Commandes AIOS (langage naturel):\n",
                                     MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print("  affiche <texte>   - afficher du texte\n");
                terminal_print("  memoire           - etat de la memoire\n");
                terminal_print("  efface            - effacer l'ecran\n");
                terminal_print("  calcule X + Y     - calculer\n");
                terminal_print("  processus         - liste des processus\n");
                terminal_print("  compile <code>    - compiler du code AIOS\n");
            } else {
                terminal_print_color("AIOS natural language commands:\n",
                                     MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print("  print/show <text> - display text\n");
                terminal_print("  memory/ram        - memory status\n");
                terminal_print("  clear             - clear screen\n");
                terminal_print("  calculate X + Y   - compute expression\n");
                terminal_print("  processes/ps      - list processes\n");
                terminal_print("  compile <code>    - compile AIOS source\n");
                terminal_print("  about/what is aios- about this OS\n");
            }
            break;

        case INTENT_ABOUT:
            terminal_print_color("AIOS - Artificial Intelligence Operating System\n",
                                 MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
            terminal_print("Built from scratch: x86 asm -> C kernel -> compiler -> AI\n");
            terminal_print("Every input routes through the AI intent engine.\n");
            terminal_print("Phase 4 active: natural language interface running.\n");
            terminal_print("Next: Phase 5 - AIOS rewrites itself in its own language.\n");
            break;

        case INTENT_CALC: {
            int result = eval_expr(intent.arg1);
            terminal_print_color("= ", MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
            terminal_print_int(result);
            terminal_newline();
            break;
        }

        case INTENT_LIST_PROC:
            syscall_dispatch(SYS_PRINT,(unsigned int)"Processes:\n",
                             MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK),0);
            process_list();
            break;

        case INTENT_COMPILE: {
            compile_result_t result;
            int r = compiler_compile(intent.arg1, &result);
            if(r==0){
                terminal_print_color("Compiled OK - running...\n",
                                     MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                process_exec_binary("ai_prog", result.code, result.size);
                kfree(result.code);
            } else {
                terminal_print_color("Error: ", MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
                terminal_print(result.errmsg);
                terminal_newline();
            }
            break;
        }

        case INTENT_AI_QUERY: {
            const char* q = input;
            if(sstart(input,"what is "))  q=input+8;
            else if(sstart(input,"what are ")) q=input+9;
            else if(sstart(input,"who is "))  q=input+7;
            char qbuf[64]; int qi=0;
            while(q[qi]&&qi<63){qbuf[qi]=q[qi];qi++;}
            qbuf[qi]=0;
            const char* kbval = kb_get(qbuf);
            if(kbval){
                terminal_print_color(qbuf,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
                terminal_print_color(" = ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
                terminal_print_color(kbval,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_newline();
            } else {
                terminal_print_color("Unknown: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print(qbuf);
                terminal_print_color("\nTry: knowledge\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            }
            break;
        }

        case INTENT_UNKNOWN:
        default:
            // Try auto-generate before giving up
            if(learning_handle(input)) break;
            terminal_print_color("I don't understand: '", MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            terminal_print(input);
            terminal_print_color("'\n", MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            terminal_print("Tip: teach aios X means Y\n");
            terminal_print("     when i say X do Y\n");
            break;
    }
    terminal_newline();
}
