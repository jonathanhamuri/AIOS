#ifndef APPS_H
#define APPS_H

#define MAX_TASKS     16
#define TASK_CMD_LEN  128

typedef struct {
    char command[TASK_CMD_LEN];
    int  interval;    // ticks between runs
    int  last_run;    // last tick it ran
    int  active;
    int  runs;        // how many times run
} scheduled_task_t;

// Built-in apps
void app_calculator();
void app_sysinfo();
void app_filelist();
void app_clock();
void app_network_status();

// Task scheduler
void scheduler_init();
int  scheduler_add(const char* cmd, int interval_seconds);
void scheduler_tick_check(int current_tick);
void scheduler_list();
void scheduler_remove(int idx);

// Script runner
void script_run(const char* script);

// Main handler
int apps_handle(const char* input);

extern scheduled_task_t task_table[MAX_TASKS];
extern int task_count;

#endif
