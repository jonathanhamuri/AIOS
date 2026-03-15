#ifndef AI_CODEGEN_H
#define AI_CODEGEN_H

// AI generates kernel modules from plain English
int ai_generate_module(const char* description, char* code_out, int max_len);
int ai_load_module(const char* description);

#endif
