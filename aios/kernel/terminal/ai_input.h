#ifndef AI_INPUT_H
#define AI_INPUT_H

// Command handler function pointer type
typedef void (*cmd_handler_t)(const char* args);

typedef struct {
    const char* name;
    const char* description;
    cmd_handler_t handler;
} ai_command_t;

void ai_init();
void ai_process_input(const char* input);

#endif
