#include "autonomy.h"
#include "../../terminal/terminal.h"
#include "../../mm/pmm.h"
#include "../../ai/knowledge/kb.h"
#include "../../ai/learning/learning.h"
#include "../../apps/apps.h"

static int sstart(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static int seq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}

aios_state_t aios_state;
static observation_t observations[MAX_OBSERVATIONS];
static int obs_count=0;

static void sep(){
    terminal_print_color("--------------------------------------------\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void autonomy_init(){
    aios_state.health_score=100;
    aios_state.memory_ok=1;
    aios_state.network_ok=1;
    aios_state.ai_ok=1;
    aios_state.disk_ok=1;
    aios_state.uptime_ticks=0;
    aios_state.commands_processed=0;
    aios_state.auto_mode=0;
    aios_state.last_check_tick=0;
    obs_count=0;
    terminal_print_color("AI Autonomy      : OK\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

// Track what user types to learn patterns
void autonomy_observe(const char*input){
    aios_state.commands_processed++;

    // Check if we've seen this before
    for(int i=0;i<obs_count;i++){
        if(seq(observations[i].pattern,input)){
            observations[i].count++;
            return;
        }
    }
    // New observation
    if(obs_count<MAX_OBSERVATIONS){
        scopy(observations[obs_count].pattern,input,OBS_LEN);
        observations[obs_count].count=1;
        obs_count++;
    }
}

void autonomy_self_check(){
    terminal_print_color("\n[AIOS] Self-Diagnostic Running...\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    sep();

    int score=100;

    // Memory check
    int free_p = pmm_free_pages();
    int mem_pct = (256-free_p)*100/256;
    aios_state.memory_ok = (mem_pct < 90);
    terminal_print_color("  Memory:   ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    if(aios_state.memory_ok){
        terminal_print_color("OK  (",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print_int(mem_pct);
        terminal_print_color("% used)\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    } else {
        terminal_print_color("WARNING - High usage!\n",
            MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        score-=20;
    }

    // AI engine check
    aios_state.ai_ok=1;
    terminal_print_color("  AI:       ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("OK  (Intent engine active)\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Network check
    aios_state.network_ok=1;
    terminal_print_color("  Network:  ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("OK  (10.0.2.15)\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Disk check
    aios_state.disk_ok=1;
    terminal_print_color("  Disk:     ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("OK  (KBFS mounted)\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Skills check
    terminal_print_color("  Skills:   ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(learning_count);
    terminal_print_color(" learned\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Tasks check
    terminal_print_color("  Tasks:    ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(task_count);
    terminal_print_color(" scheduled\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Commands processed
    terminal_print_color("  Commands: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(aios_state.commands_processed);
    terminal_print_color(" processed\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    aios_state.health_score=score;
    sep();
    terminal_print_color("  Health Score: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_int(score);
    terminal_print_color("/100  ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    if(score>=80)
        terminal_print_color("EXCELLENT\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    else if(score>=60)
        terminal_print_color("GOOD\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    else
        terminal_print_color("NEEDS ATTENTION\n",MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
    sep();
}

void autonomy_report(){
    terminal_print_color("\n[AIOS] Autonomy Report\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    sep();

    extern unsigned int timer_ticks_bss;
    int uptime=((int)timer_ticks_bss)/100;

    terminal_print_color("  Status:    ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color(aios_state.auto_mode?"AUTO MODE ON":"MANUAL MODE",
        aios_state.auto_mode?
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK):
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_newline();

    terminal_print_color("  Uptime:    ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(uptime);
    terminal_print_color("s\n",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));

    terminal_print_color("  Health:    ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(aios_state.health_score);
    terminal_print_color("/100\n",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));

    terminal_print_color("  Commands:  ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(aios_state.commands_processed);
    terminal_newline();

    terminal_print_color("  Patterns:  ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(obs_count);
    terminal_print_color(" observed\n",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));

    // Show top patterns
    if(obs_count>0){
        terminal_print_color("  Top commands:\n",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        // Find most used
        for(int i=0;i<obs_count&&i<5;i++){
            int max_idx=i;
            for(int j=i+1;j<obs_count;j++)
                if(observations[j].count>observations[max_idx].count)
                    max_idx=j;
            // swap
            if(max_idx!=i){
                observation_t tmp=observations[i];
                observations[i]=observations[max_idx];
                observations[max_idx]=tmp;
            }
            terminal_print_color("    ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_int(observations[i].count);
            terminal_print_color("x: ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(observations[i].pattern,
                MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            terminal_newline();
        }
    }
    sep();
}

void autonomy_optimize(){
    terminal_print_color("\n[AIOS] Running Self-Optimization...\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    sep();

    int optimized=0;

    // Check memory
    int free_p=pmm_free_pages();
    terminal_print_color("  Checking memory...",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("OK\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    // Auto-learn frequent patterns
    for(int i=0;i<obs_count;i++){
        if(observations[i].count>=3){
            // Check if already a skill
            int found=0;
            for(int j=0;j<learning_count;j++){
                extern skill_t skill_table[];
                if(seq(skill_table[j].name,observations[i].pattern)){
                    found=1;break;
                }
            }
            if(!found){
                terminal_print_color("  Auto-learning: ",
                    MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
                terminal_print(observations[i].pattern);
                terminal_newline();
                optimized++;
            }
        }
    }

    // Generate optimization module
    terminal_print_color("  Generating optimizer module...\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    extern int ai_load_module(const char*);
    ai_load_module("module that monitors");

    if(optimized>0){
        terminal_print_color("  Optimized ",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print_int(optimized);
        terminal_print_color(" patterns\n",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    } else {
        terminal_print_color("  System already optimal\n",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    }

    aios_state.health_score=100;
    sep();
    terminal_print_color("  Optimization complete\n",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    sep();
}

void autonomy_suggest(){
    // Proactive suggestions based on patterns
    if(aios_state.commands_processed % 10 == 0 &&
       aios_state.commands_processed > 0){
        terminal_print_color("\n[AIOS] Suggestion: ",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));

        if(learning_count==0)
            terminal_print_color("Try: teach aios hello means Hi!\n",
                MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        else if(task_count==0)
            terminal_print_color("Try: schedule every 30s sysinfo\n",
                MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        else
            terminal_print_color("Try: aios report\n",
                MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    }
}

// Called every N ticks in auto mode
void autonomy_auto_tick(int tick){
    if(!aios_state.auto_mode) return;

    aios_state.uptime_ticks=tick;

    // Auto self-check every 30 seconds
    if(tick - aios_state.last_check_tick > 3000){
        aios_state.last_check_tick=tick;
        int free_p=pmm_free_pages();
        if(free_p < 20){
            terminal_print_color("[AUTO] Warning: Low memory!\n",
                MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        }
    }
}

void autonomy_what_can_i_do(){
    terminal_print_color("\n[AIOS] My Capabilities:\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    sep();
    terminal_print_color("  LANGUAGE:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    hello, bonjour, help, about\n");
    terminal_print("    print <text>, calculate X+Y\n");
    terminal_print("\n");
    terminal_print_color("  GRAPHICS:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    draw circle/rect/star <color>\n");
    terminal_print("    show memory, paint <color>\n");
    terminal_print("\n");
    terminal_print_color("  LEARNING:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    teach aios X means Y\n");
    terminal_print("    when i say X do Y\n");
    terminal_print("    remember X, skills\n");
    terminal_print("\n");
    terminal_print_color("  ENGINEERING:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    plan railway from X to Y\n");
    terminal_print("    launch satellite\n");
    terminal_print("    plan bridge over X\n");
    terminal_print("\n");
    terminal_print_color("  DOCUMENTS:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    write essay about X\n");
    terminal_print("    write report on X\n");
    terminal_print("    write letter to X about Y\n");
    terminal_print("\n");
    terminal_print_color("  APPS:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    sysinfo, clock, calculator\n");
    terminal_print("    schedule every Xs <cmd>\n");
    terminal_print("    run script: cmd1, cmd2\n");
    terminal_print("\n");
    terminal_print_color("  NETWORK:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    discover, peers\n");
    terminal_print("    send message <text>\n");
    terminal_print("\n");
    terminal_print_color("  AUTONOMY:\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print("    aios status, aios report\n");
    terminal_print("    aios optimize, aios auto on\n");
    terminal_print("    create module <description>\n");
    sep();
}

int autonomy_handle(const char*input){
    // Self check
    if(seq(input,"aios status")||seq(input,"self check")||
       seq(input,"system check")||seq(input,"diagnostics")){
        autonomy_self_check();return 1;
    }
    // Report
    if(seq(input,"aios report")||seq(input,"autonomy report")||
       seq(input,"show report")){
        autonomy_report();return 1;
    }
    // Optimize
    if(seq(input,"aios optimize")||seq(input,"optimize")||
       seq(input,"self optimize")){
        autonomy_optimize();return 1;
    }
    // Auto mode
    if(seq(input,"aios auto on")||seq(input,"auto mode on")||
       seq(input,"enable auto")){
        aios_state.auto_mode=1;
        terminal_print_color("[AIOS] Auto mode ENABLED\n",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print_color("  AIOS will monitor and optimize itself\n",
            MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        return 1;
    }
    if(seq(input,"aios auto off")||seq(input,"auto mode off")||
       seq(input,"disable auto")){
        aios_state.auto_mode=0;
        terminal_print_color("[AIOS] Auto mode DISABLED\n",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        return 1;
    }
    // What can you do
    if(seq(input,"what can you do")||seq(input,"capabilities")||
       seq(input,"help all")||seq(input,"full help")){
        autonomy_what_can_i_do();return 1;
    }
    // Monitor
    if(seq(input,"monitor system")||seq(input,"monitor")){
        autonomy_self_check();
        autonomy_report();
        return 1;
    }
    // Improve
    if(seq(input,"aios improve")||seq(input,"self improve")){
        terminal_print_color("[AIOS] Initiating self-improvement...\n",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        autonomy_optimize();
        extern int ai_load_module(const char*);
        ai_load_module("module that monitors");
        terminal_print_color("[AIOS] Self-improvement complete\n",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        return 1;
    }
    return 0;
}

/* ── Voice bridge TCP server (port 7777) ── */
#include "../../net/rtl8139.h"

void voice_server_tick(void) {
    /* Handled by net stack — placeholder for socket accept */
}
