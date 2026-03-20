#include "voice_server.h"
#include "../terminal/terminal.h"
#include "../ai/ai_exec.h"

char voice_last_command[128] = {0};
char voice_last_response[512] = {0};
int  voice_command_ready = 0;

/* Simple ring buffer for voice commands coming in via serial/net */
static char cmd_buf[128];
static int  cmd_len = 0;

void voice_server_init(void){
    terminal_print_color("Voice server ready\n", 2);
}

void voice_server_tick(void){
    if(voice_command_ready){
        /* Route to AI */
        extern void ai_process_input(const char*);
        ai_process_input(voice_last_command);
        voice_command_ready = 0;
    }
}
