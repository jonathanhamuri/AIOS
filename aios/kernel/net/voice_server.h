#ifndef VOICE_SERVER_H
#define VOICE_SERVER_H
void voice_server_init(void);
void voice_server_tick(void);
extern char voice_last_command[128];
extern char voice_last_response[512];
extern int  voice_command_ready;
#endif
