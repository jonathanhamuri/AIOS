#ifndef KB_H
#define KB_H

#define KB_MAX_FACTS    128
#define KB_MAX_COMMANDS  32
#define KB_KEY_LEN       64
#define KB_VAL_LEN      256

// A fact: key=value pair stored in kernel memory
typedef struct {
    char key[KB_KEY_LEN];
    char value[KB_VAL_LEN];
    int  active;
} kb_fact_t;

// A learned command: phrase -> aios source code
typedef struct {
    char trigger[KB_KEY_LEN];   // what the user types
    char source[KB_VAL_LEN];    // AIOS source to compile and run
    int  active;
} kb_command_t;

void        kb_init();
void        kb_set(const char* key, const char* val);
const char* kb_get(const char* key);
void        kb_delete(const char* key);
void        kb_list();

// Learned commands
int  kb_learn(const char* trigger, const char* source);
int  kb_recall(const char* input);   // returns 1 if a learned command fired
void kb_list_commands();

#endif

// Access learned command source by index
const char* commands_get_source(int idx);
const char* commands_get_trigger(int idx);

// Index-based accessors for disk persistence
const char* kb_get_key(int idx);
const char* kb_get_val(int idx);
int         kb_get_active(int idx);
int         commands_get_active(int idx);
