#include "learning.h"
#include "../../terminal/terminal.h"
#include "../../compiler/compiler.h"
#include "../../process/process.h"
#include "../../mm/heap.h"
#include "../../ai/knowledge/kb.h"
#include "../../disk/kbfs.h"
#include "../codegen/ai_codegen.h"

int learning_count = 0;
skill_t skill_table[MAX_SKILLS];

static int slen(const char* s){int n=0;while(*s++)n++;return n;}
static int seq(const char* a,const char* b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int sstart(const char* s,const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static void scopy(char* d,const char* s,int max){
    int i=0;while(s[i]&&i<max-1){d[i]=s[i];i++;}d[i]=0;
}
static void sappend(char* d,const char* s){
    int i=slen(d),j=0;
    while(s[j]){d[i++]=s[j++];}d[i]=0;
}

void learning_init(){
    learning_count=0;
    learning_load();
    terminal_print_color("Auto-learning    : OK\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

// Try to auto-generate code for unknown input
static int auto_generate(const char* input, char* code_out){
    code_out[0]=0;

    // "teach me to X" / "learn X"
    // "when I say X do Y"
    if(sstart(input,"when i say ")){
        // "when i say X do Y"
        const char* p = input+11;
        char trigger[64]={0};
        int i=0;
        while(p[i]&&!sstart(p+i," do ")&&i<63){
            trigger[i]=p[i];i++;
        }
        trigger[i]=0;
        const char* action = p+i+4;
        // Generate print code for the action
        sappend(code_out,"print \"");
        sappend(code_out,action);
        sappend(code_out,"\";");
        // Save as skill with trigger as name
        learning_add(trigger,code_out);
        terminal_print_color("[LEARN] Learned: '",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(trigger);
        terminal_print_color("' -> '",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(action);
        terminal_print_color("'\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        return 1;
    }

    // "teach aios X means Y"
    if(sstart(input,"teach aios ")||sstart(input,"teach me ")){
        const char* p = sstart(input,"teach aios ") ? input+11 : input+9;
        char name[64]={0};
        int i=0;
        while(p[i]&&!sstart(p+i," means ")&&!sstart(p+i," is ")&&i<63){
            name[i]=p[i];i++;
        }
        name[i]=0;
        const char* val = p+i;
        while(*val&&(*val==' '||sstart(val,"means ")||sstart(val,"is "))){
            if(sstart(val,"means ")) val+=6;
            else if(sstart(val,"is ")) val+=3;
            else val++;
        }
        sappend(code_out,"print \"");
        sappend(code_out,val);
        sappend(code_out,"\";");
        learning_add(name,code_out);
        terminal_print_color("[LEARN] Learned: '",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(name);
        terminal_print_color("'\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        return 1;
    }

    // "remember X"
    if(sstart(input,"remember ")){
        const char* msg = input+9;
        sappend(code_out,"print \"[MEMORY] ");
        sappend(code_out,msg);
        sappend(code_out,"\";");
        char key[64]="mem:";
        sappend(key,msg);
        learning_add(key,code_out);
        kb_set(key,msg);
        terminal_print_color("[LEARN] Remembered: ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(msg);
        terminal_newline();
        return 1;
    }

    // "forget X"
    if(sstart(input,"forget ")){
        const char* what = input+7;
        terminal_print_color("[LEARN] Forgetting: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print(what);
        terminal_newline();
        // Remove from skill table
        for(int i=0;i<learning_count;i++){
            if(seq(skill_table[i].name,what)){
                for(int j=i;j<learning_count-1;j++)
                    skill_table[j]=skill_table[j+1];
                learning_count--;
                break;
            }
        }
        learning_save();
        return 1;
    }

    // "skills" / "what do you know"
    if(seq(input,"skills")||seq(input,"what do you know")||
       sstart(input,"list skills")){
        learning_list();
        return 1;
    }

    return 0;
}

int learning_add(const char* name, const char* code){
    if(learning_count>=MAX_SKILLS) return -1;
    // Check if exists - update
    for(int i=0;i<learning_count;i++){
        if(seq(skill_table[i].name,name)){
            scopy(skill_table[i].code,code,SKILL_CODE_LEN);
            skill_table[i].hits=0;
            learning_save();
            return 0;
        }
    }
    scopy(skill_table[learning_count].name,name,SKILL_NAME_LEN);
    scopy(skill_table[learning_count].code,code,SKILL_CODE_LEN);
    skill_table[learning_count].hits=0;
    learning_count++;
    learning_save();
    return 0;
}

int learning_handle(const char* input){
    // First check if we have a learned skill matching this input
    for(int i=0;i<learning_count;i++){
        if(seq(skill_table[i].name,input)||
           sstart(input,skill_table[i].name)){
            skill_table[i].hits++;
            terminal_print_color("[SKILL] Running: ",
                MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
            terminal_print(skill_table[i].name);
            terminal_newline();
            // Compile and run
            compile_result_t result;
            if(compiler_compile(skill_table[i].code,&result)==0){
                process_exec_binary("skill",result.code,result.size);
                kfree(result.code);
            }
            return 1;
        }
    }

    // Try auto-generate
    char code[SKILL_CODE_LEN]={0};
    if(auto_generate(input,code)){
        if(code[0]){
            compile_result_t result;
            if(compiler_compile(code,&result)==0){
                process_exec_binary("skill",result.code,result.size);
                kfree(result.code);
            }
        }
        return 1;
    }

    return 0;
}

void learning_list(){
    terminal_print_color("[SKILLS] Learned skills:\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    if(learning_count==0){
        terminal_print("  No skills learned yet.\n");
        terminal_print("  Try: teach aios hello means Hi there!\n");
        terminal_print("  Or:  when i say bye do Goodbye friend!\n");
        return;
    }
    for(int i=0;i<learning_count;i++){
        terminal_print_color("  ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        terminal_print_color(skill_table[i].name,
            MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_color(" -> ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print(skill_table[i].code);
        terminal_print_color(" [used:",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(skill_table[i].hits);
        terminal_print_color("x]\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    }
}

// Save skills to KBFS
void learning_save(){
    char buf[16];
    buf[0]='0'+learning_count/10;
    buf[1]='0'+learning_count%10;
    buf[2]=0;
    kb_set("_skill_count",buf);

    for(int i=0;i<learning_count;i++){
        char key[32];
        key[0]='_'; key[1]='s'; key[2]='k';
        key[3]='_'; key[4]='0'+i/10; key[5]='0'+i%10;
        key[6]='_'; key[7]='n'; key[8]=0;
        kb_set(key,skill_table[i].name);
        key[7]='c'; key[8]=0;
        kb_set(key,skill_table[i].code);
    }
    // Persist to disk
    kbfs_save();
    terminal_print_color("[LEARN] Saved to disk\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
}

// Load skills from KBFS
void learning_load(){
    const char* cnt = kb_get("_skill_count");
    if(!cnt) return;
    int count = (cnt[0]-'0')*10+(cnt[1]-'0');
    if(count>MAX_SKILLS) count=MAX_SKILLS;

    for(int i=0;i<count;i++){
        char key[32];
        key[0]='_'; key[1]='s'; key[2]='k';
        key[3]='_'; key[4]='0'+i/10; key[5]='0'+i%10;
        key[6]='_'; key[7]='n'; key[8]=0;
        const char* name = kb_get(key);
        key[7]='c'; key[8]=0;
        const char* code = kb_get(key);
        if(name&&code){
            scopy(skill_table[i].name,name,SKILL_NAME_LEN);
            scopy(skill_table[i].code,code,SKILL_CODE_LEN);
            skill_table[i].hits=0;
            learning_count++;
        }
    }
    if(learning_count>0){
        terminal_print_color("[LEARN] Loaded ",
            MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(learning_count);
        terminal_print_color(" skills\n",
            MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    }
}
