#include "apps.h"
#include "../terminal/terminal.h"
#include "../graphics/vga.h"
#include "../mm/pmm.h"
#include "../ai/knowledge/kb.h"
#include "../ai/learning/learning.h"

static int sstart(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static int seq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}
static int satoi(const char*s){int n=0;while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;}return n;}

scheduled_task_t task_table[MAX_TASKS];
int task_count=0;

// ── Built-in Apps ─────────────────────────────────────────

void app_calculator(){
    terminal_print_color("\n[CALC] AIOS Calculator\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  Supported: +  -  *  /\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Usage: calculate 5 + 3\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("         calculate 100 * 25\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("         calculate 1000 / 4\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void app_sysinfo(){
    terminal_print_color("\n[SYSINFO] AIOS System Information\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  OS:       AIOS v1.0\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Arch:     x86 32-bit protected mode\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Kernel:   C + Assembly\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  AI:       Intent Engine Phase 13+\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));

    int free_p = pmm_free_pages();
    terminal_print_color("  Memory:   ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(free_p*4);
    terminal_print_color(" KB free\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    extern unsigned int timer_ticks_bss;
    int uptime = (int)timer_ticks_bss/100;
    terminal_print_color("  Uptime:   ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(uptime);
    terminal_print_color(" seconds\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    terminal_print_color("  Network:  10.0.2.15 (RTL8139)\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));

    extern int learning_count;
    terminal_print_color("  Skills:   ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(learning_count);
    terminal_print_color(" learned\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    terminal_print_color("  Tasks:    ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(task_count);
    terminal_print_color(" scheduled\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void app_clock(){
    extern unsigned int timer_ticks_bss;
    int t = (int)timer_ticks_bss/100;
    int sec = t%60;
    int min = (t/60)%60;
    int hr  = (t/3600)%24;
    terminal_print_color("\n[CLOCK] AIOS System Time\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  Time: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(hr);terminal_print_color(":",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    if(min<10) terminal_print_color("0",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_int(min);terminal_print_color(":",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    if(sec<10) terminal_print_color("0",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_int(sec);terminal_newline();
    terminal_print_color("  Ticks: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int((int)timer_ticks_bss);terminal_newline();
}

void app_network_status(){
    terminal_print_color("\n[NET] Network Status\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  Interface: RTL8139\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  IP:        10.0.2.15\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Gateway:   10.0.2.2\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Status:    ONLINE\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    extern int peer_count;
    terminal_print_color("  Peers:     ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(peer_count);
    terminal_print_color(" connected\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void app_filelist(){
    terminal_print_color("\n[FILES] AIOS File System (KBFS)\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  /kb/        Knowledge base\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  /skills/    Learned skills\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  /docs/      Generated documents\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  /net/       Network config\n",
        MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color("  Use 'knowledge' to list KB entries\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("  Use 'skills' to list learned skills\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

// ── Task Scheduler ────────────────────────────────────────

void scheduler_init(){
    task_count=0;
    for(int i=0;i<MAX_TASKS;i++) task_table[i].active=0;
    terminal_print_color("Task Scheduler   : OK\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

int scheduler_add(const char*cmd, int interval_sec){
    if(task_count>=MAX_TASKS) return -1;
    int interval_ticks = interval_sec*100;
    scheduled_task_t*t=&task_table[task_count];
    scopy(t->command,cmd,TASK_CMD_LEN);
    t->interval=interval_ticks;
    t->last_run=0;
    t->active=1;
    t->runs=0;
    task_count++;
    terminal_print_color("[TASK] Scheduled: ",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print(cmd);
    terminal_print_color(" every ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    terminal_print_int(interval_sec);
    terminal_print_color("s\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    return task_count-1;
}

void scheduler_tick_check(int current_tick){
    for(int i=0;i<task_count;i++){
        if(!task_table[i].active) continue;
        if(current_tick - task_table[i].last_run >= task_table[i].interval){
            task_table[i].last_run = current_tick;
            task_table[i].runs++;
            // Execute the command via AI
            extern void ai_process_input(const char* input);
            ai_process_input(task_table[i].command);
        }
    }
}

void scheduler_list(){
    terminal_print_color("[TASKS] Scheduled tasks:\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    if(task_count==0){
        terminal_print("  No tasks scheduled.\n");
        terminal_print("  Use: schedule every 10s <command>\n");
        return;
    }
    for(int i=0;i<task_count;i++){
        if(!task_table[i].active) continue;
        terminal_print_color("  [",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(i);
        terminal_print_color("] ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print(task_table[i].command);
        terminal_print_color(" | every ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(task_table[i].interval/100);
        terminal_print_color("s | ran ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(task_table[i].runs);
        terminal_print_color("x\n",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    }
}

void scheduler_remove(int idx){
    if(idx<0||idx>=task_count) return;
    task_table[idx].active=0;
    terminal_print_color("[TASK] Removed task ",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_int(idx);
    terminal_newline();
}

// ── Script Runner ─────────────────────────────────────────

void script_run(const char*script){
    terminal_print_color("[SCRIPT] Running script...\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    extern void ai_process_input(const char*);

    char cmd[TASK_CMD_LEN]={0};
    int ci=0;
    const char*p=script;

    while(*p){
        if(*p==',' || *p==';' || (*p=='t'&&*(p+1)=='h'&&*(p+2)=='e'&&*(p+3)=='n')){
            if(ci>0){
                cmd[ci]=0;
                // trim leading spaces
                char*c=cmd;
                while(*c==' ')c++;
                if(*c){
                    terminal_print_color("[SCRIPT] -> ",
                        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
                    terminal_print(c);terminal_newline();
                    ai_process_input(c);
                }
                ci=0;
            }
            // skip 'then'
            if(*p=='t') p+=4;
            else p++;
            continue;
        }
        if(ci<TASK_CMD_LEN-1) cmd[ci++]=*p;
        p++;
    }
    // Last command
    if(ci>0){
        cmd[ci]=0;
        char*c=cmd;
        while(*c==' ')c++;
        if(*c){
            terminal_print_color("[SCRIPT] -> ",
                MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
            terminal_print(c);terminal_newline();
            ai_process_input(c);
        }
    }
    terminal_print_color("[SCRIPT] Done\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
}

// ── Main Handler ─────────────────────────────────────────

int apps_handle(const char*input){
    // App launcher
    if(seq(input,"calculator")||seq(input,"run calculator")){
        app_calculator();return 1;
    }
    if(seq(input,"sysinfo")||seq(input,"system info")||
       seq(input,"run sysinfo")){
        app_sysinfo();return 1;
    }
    if(seq(input,"clock")||seq(input,"show clock")||
       seq(input,"run clock")){
        app_clock();return 1;
    }
    if(seq(input,"network status")||seq(input,"run network")||
       seq(input,"net info")){
        app_network_status();return 1;
    }
    if(seq(input,"files")||seq(input,"file list")||
       seq(input,"run files")){
        app_filelist();return 1;
    }

    // Task scheduler
    if(sstart(input,"schedule every ")){
        // "schedule every 10s <command>"
        // "schedule every 10 seconds <command>"
        const char*p=input+15;
        int secs=satoi(p);
        if(secs<=0) secs=10;
        // skip number
        while(*p>='0'&&*p<='9')p++;
        // skip 's' or ' seconds'
        while(*p==' '||*p=='s')p++;
        if(sstart(p,"seconds "))p+=8;
        if(sstart(p,"second "))p+=7;
        if(*p) scheduler_add(p,secs);
        return 1;
    }

    if(seq(input,"tasks")||seq(input,"list tasks")||
       seq(input,"show tasks")){
        scheduler_list();return 1;
    }

    if(sstart(input,"remove task ")){
        int idx=satoi(input+12);
        scheduler_remove(idx);return 1;
    }

    if(sstart(input,"stop task ")){
        int idx=satoi(input+10);
        scheduler_remove(idx);return 1;
    }

    // Script runner
    if(sstart(input,"run script:")||sstart(input,"script:")){
        const char*s=input;
        if(sstart(s,"run script:"))s+=11;
        else if(sstart(s,"script:"))s+=7;
        while(*s==' ')s++;
        script_run(s);return 1;
    }

    if(sstart(input,"chain:")||sstart(input,"chain ")){
        const char*s=sstart(input,"chain:")?input+6:input+6;
        while(*s==' ')s++;
        script_run(s);return 1;
    }

    // Apps help
    if(seq(input,"apps")||seq(input,"app list")){
        terminal_print_color("[APPS] Available:\n",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print("  calculator, sysinfo, clock\n");
        terminal_print("  network status, files\n");
        terminal_print("  schedule every 10s <cmd>\n");
        terminal_print("  tasks, remove task <n>\n");
        terminal_print("  run script: cmd1, cmd2, cmd3\n");
        terminal_print("  chain: cmd1 then cmd2 then cmd3\n");
        return 1;
    }

    return 0;
}
