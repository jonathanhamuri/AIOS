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

        case INTENT_GREET: {
            const char* ver = kb_get("version");
            const char* mods = kb_get("modules");
            if(seq(intent.lang,"fr")){
                terminal_print_color("Bonjour! Je suis AIMERANCIA v",
                    MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color(ver?ver:"1.0",
                    MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print_color("\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color("Systeme d exploitation IA natif sur x86 32-bit.\n",
                    MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_print_color("Modules actifs: ",
                    MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
                terminal_print_color(mods?mods:"tous",
                    MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_newline();
            } else {
                terminal_print_color("Hello! I am AIMERANCIA v",
                    MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color(ver?ver:"1.0",
                    MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print_color("\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print_color("AI-native operating system on x86 32-bit.\n",
                    MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_print_color("Active modules: ",
                    MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
                terminal_print_color(mods?mods:"all",
                    MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_newline();
            }
            break;
        }

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
                terminal_print_color("=== AIMERANCIA — Commandes ===\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print("  affiche <texte>              - afficher du texte\n");
                terminal_print("  memoire                      - etat memoire\n");
                terminal_print("  calcule X + Y                - calcul\n");
                terminal_print("  processus                    - liste processus\n");
                terminal_print("  about / qui es-tu            - info systeme\n");
                terminal_print("  plan railway from X to Y     - ingenierie ferroviaire\n");
                terminal_print("  build bridge over X          - ingenierie pont\n");
                terminal_print("  launch satellite             - calcul orbital\n");
                terminal_print("  scan network                 - decouverte reseau\n");
                terminal_print("  generate report              - generer rapport\n");
                terminal_print("  teach aios X means Y         - apprentissage\n");
                terminal_print("  auto mode on                 - mode autonome\n");
                terminal_print("  status report                - rapport complet\n");
                terminal_print("  compile <code>               - compiler code AIOS\n");
                terminal_print("  sleep now / shutdown         - eteindre\n");
            } else {
                terminal_print_color("=== AIMERANCIA — Commands ===\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                terminal_print("  print/show <text>            - display text\n");
                terminal_print("  memory/ram                   - memory status\n");
                terminal_print("  calculate X + Y              - compute\n");
                terminal_print("  processes/ps                 - list processes\n");
                terminal_print("  about / who are you          - system info\n");
                terminal_print("  plan railway from X to Y     - railway engineering\n");
                terminal_print("  build bridge over X          - bridge engineering\n");
                terminal_print("  launch satellite             - orbital mechanics\n");
                terminal_print("  scan network                 - discover devices\n");
                terminal_print("  generate report              - create report\n");
                terminal_print("  teach aios X means Y         - learning\n");
                terminal_print("  auto mode on                 - autonomous mode\n");
                terminal_print("  status report                - full system report\n");
                terminal_print("  compile <code>               - compile AIOS source\n");
                terminal_print("  sleep now / shutdown         - power off\n");
            }
            break;

        case INTENT_ABOUT: {
            terminal_print_color("=== AIMERANCIA ===\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            const char* fields[] = {
                "name","version","arch","language","memory_mgr",
                "bootloader","display","net_driver","filesystem",
                "ai_pipeline","engineering","learning","autonomy",
                "bare_metal",0};
            for(int i=0;fields[i];i++){
                const char* v=kb_get(fields[i]);
                if(v){
                    terminal_print_color(fields[i],MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
                    terminal_print_color(": ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
                    terminal_print_color(v,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                    terminal_newline();
                }
            }
            break;
        }

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
        default: {
            if(learning_handle(input)) break;
            /* Try KB lookup */
            const char* kbv = kb_get(input);
            if(kbv){
                terminal_print_color(input,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
                terminal_print_color(": ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
                terminal_print_color(kbv,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
                terminal_newline();
                break;
            }
            terminal_print_color("Unknown command: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            terminal_print(input);
            terminal_newline();
            terminal_print_color("Say 'help' for all commands\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color("Say 'about' for system info\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color("Say 'teach aios X means Y' to add skills\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            break;
        }
    }
    terminal_newline();
}
