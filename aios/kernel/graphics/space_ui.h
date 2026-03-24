#ifndef SPACE_UI_H
#define SPACE_UI_H

/* Planet IDs */
#define PLANET_NONE      -1
#define PLANET_DOCUMENTS  0
#define PLANET_LEARNING   1
#define PLANET_NETWORK    2
#define PLANET_ENGINEERING 3
#define PLANET_AUTONOMY   4
#define PLANET_CODE       5
#define PLANET_SCHEDULER  6

void space_ui_init(void);
void space_ui_tick(void);
void space_ui_draw(void);
int  space_ui_active(void);
void space_ui_travel_to(int planet_id);
void space_ui_return(void);
int  space_ui_handle(const char* input);
void space_ui_set_speaking(int on);
extern int space_mode;

#endif
