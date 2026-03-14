#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH   320
#define VGA_HEIGHT  200
#define VGA_MEMORY  0xA0000

#define COL_BLACK    0
#define COL_BLUE     1
#define COL_GREEN    2
#define COL_CYAN     3
#define COL_RED      4
#define COL_YELLOW   14
#define COL_WHITE    15
#define COL_LGRAY    7
#define COL_DGRAY    8

void vga_init();
void vga_clear(unsigned char color);
void vga_putpixel(int x, int y, unsigned char color);
void vga_rectfill(int x, int y, int w, int h, unsigned char color);
void vga_rect(int x, int y, int w, int h, unsigned char color);
void vga_line(int x0, int y0, int x1, int y1, unsigned char color);
void vga_drawchar(int x, int y, char c, unsigned char fg, unsigned char bg);
void vga_drawstring(int x, int y, const char* s, unsigned char fg, unsigned char bg);
void vga_drawint(int x, int y, int val, unsigned char fg, unsigned char bg);
void vga_shell_init();
void vga_shell_print(const char* s, unsigned char color);
void vga_shell_prompt();
void vga_shell_newline();

extern int vga_active;
extern int shell_cursor_x;
extern int shell_cursor_y;

#endif
