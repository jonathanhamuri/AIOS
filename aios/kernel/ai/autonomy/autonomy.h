#ifndef AUTONOMY_H
#define AUTONOMY_H

#define MAX_OBSERVATIONS  32
#define OBS_LEN           64

typedef struct {
    char pattern[OBS_LEN];   // what was typed
    int  count;              // how many times
    int  last_tick;
} observation_t;

typedef struct {
    int  health_score;       // 0-100
    int  memory_ok;
    int  network_ok;
    int  ai_ok;
    int  disk_ok;
    int  uptime_ticks;
    int  commands_processed;
    int  auto_mode;          // autonomous mode on/off
    int  last_check_tick;
} aios_state_t;

void autonomy_init();
int  autonomy_handle(const char* input);
void autonomy_observe(const char* input);
void autonomy_self_check();
void autonomy_report();
void autonomy_optimize();
void autonomy_auto_tick(int tick);
void autonomy_suggest();

extern aios_state_t aios_state;

#endif
