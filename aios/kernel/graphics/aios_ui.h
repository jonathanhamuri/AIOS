#ifndef AIOS_UI_H
#define AIOS_UI_H

void aios_ui_init();
void aios_ui_draw();
void aios_ui_set_status(const char* status);
void aios_ui_print(const char* text, unsigned char color);
void aios_ui_prompt();
void aios_ui_input_char(char c);
void aios_ui_input_backspace();
void aios_ui_input_clear();
void aios_ui_tick();  // called every timer tick for animations

extern int aios_ui_active;

#endif
