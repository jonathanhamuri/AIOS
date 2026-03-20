#ifndef AIOS_UI_H
#define AIOS_UI_H
void aios_ui_init(void);
void aios_ui_draw(void);
void aios_ui_tick(void);
void aios_ui_set_status(const char* s);
void aios_ui_print(const char* t, unsigned char c);
void aios_ui_prompt(void);
void aios_ui_input_char(char c);
void aios_ui_input_backspace(void);
void aios_ui_input_clear(void);
const char* aios_ui_get_input(void);
extern int aios_ui_active;
#endif
