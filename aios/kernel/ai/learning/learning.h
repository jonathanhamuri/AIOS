#ifndef LEARNING_H
#define LEARNING_H

#define MAX_SKILLS     64
#define SKILL_NAME_LEN 64
#define SKILL_CODE_LEN 256

typedef struct {
    char name[SKILL_NAME_LEN];
    char code[SKILL_CODE_LEN];
    int  hits;
} skill_t;

void learning_init();
int  learning_handle(const char* input);
int  learning_add(const char* name, const char* code);
void learning_list();
void learning_save();
void learning_load();

extern int learning_count;
extern skill_t skill_table[MAX_SKILLS];

#endif
