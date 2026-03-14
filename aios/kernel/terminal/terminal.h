#ifndef TERMINAL_H
#define TERMINAL_H

#define VGA_BASE    0xB8000
#define VGA_COLS    80
#define VGA_ROWS    25
#define VGA_SCROLL  20      // Keep this many rows on scroll
#define CMD_BUF_MAX 256     // Max command length

// Colors
#define COLOR_BLACK   0x0
#define COLOR_BLUE    0x1
#define COLOR_GREEN   0x2
#define COLOR_CYAN    0x3
#define COLOR_RED     0x4
#define COLOR_MAGENTA 0x5
#define COLOR_BROWN   0x6
#define COLOR_WHITE   0x7
#define COLOR_GRAY    0x8
#define COLOR_BBLUE   0x9
#define COLOR_BGREEN  0xA
#define COLOR_BCYAN   0xB
#define COLOR_BRED    0xC
#define COLOR_BMAGENTA 0xD
#define COLOR_BYELLOW 0xE
#define COLOR_BWHITE  0xF

#define MAKE_COLOR(fg, bg) ((bg << 4) | fg)

// Terminal state
typedef struct {
    int col;
    int row;
    unsigned char color;
    char cmd_buf[CMD_BUF_MAX];
    int  cmd_len;
    int  ai_mode;           // 1 = AI is processing input
} terminal_t;

void terminal_init();
void terminal_putchar(char c);
void terminal_print(const char* s);
void terminal_print_color(const char* s, unsigned char color);
void terminal_print_int(int n);
void terminal_print_hex(unsigned int n);
void terminal_newline();
void terminal_scroll();
void terminal_clear();
void terminal_set_color(unsigned char color);
void terminal_handle_key(char c);
void terminal_render_prompt();

// AI interface — called when user hits Enter
void ai_process_input(const char* input);

#endif
