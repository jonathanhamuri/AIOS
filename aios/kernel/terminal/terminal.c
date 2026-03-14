#include "../graphics/vga.h"
#include "terminal.h"
#include "../mm/heap.h"

static unsigned short* vga = (unsigned short*)VGA_BASE;

#define SERIAL 0x3F8

static void serial_init() {
    unsigned char v;
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x00), "Nd"((unsigned short)(SERIAL+1)));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x80), "Nd"((unsigned short)(SERIAL+3)));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x01), "Nd"((unsigned short)(SERIAL+0)));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x00), "Nd"((unsigned short)(SERIAL+1)));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x03), "Nd"((unsigned short)(SERIAL+3)));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0xC7), "Nd"((unsigned short)(SERIAL+2)));
}

static void serial_putchar(char c) {
    unsigned char status;
    do {
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((unsigned short)(SERIAL+5)));
    } while (!(status & 0x20));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)c), "Nd"((unsigned short)SERIAL));
    if (c == '\n') serial_putchar('\r');
}

static terminal_t term;

// Scroll buffer — stores last 100 lines for history
#define SCROLL_BUF_LINES 100
#define SCROLL_BUF_SIZE  (SCROLL_BUF_LINES * VGA_COLS)
static unsigned short scroll_buf[SCROLL_BUF_SIZE];
static int scroll_buf_line = 0;

static void vga_set(int row, int col, char c, unsigned char color) {
    vga[row * VGA_COLS + col] = (unsigned short)c | ((unsigned short)color << 8);
}

static void vga_copy_row(int dst, int src) {
    for (int i = 0; i < VGA_COLS; i++)
        vga[dst * VGA_COLS + i] = vga[src * VGA_COLS + i];
}

static void vga_clear_row(int row, unsigned char color) {
    for (int i = 0; i < VGA_COLS; i++)
        vga[row * VGA_COLS + i] = (unsigned short)' ' | ((unsigned short)color << 8);
}

void terminal_scroll() {
    // Save top row to scroll buffer
    for (int i = 0; i < VGA_COLS; i++)
        scroll_buf[(scroll_buf_line % SCROLL_BUF_LINES) * VGA_COLS + i] = vga[i];
    scroll_buf_line++;

    // Shift all rows up by one
    for (int r = 0; r < VGA_ROWS - 1; r++)
        vga_copy_row(r, r + 1);

    // Clear bottom row
    vga_clear_row(VGA_ROWS - 1, MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
    term.row = VGA_ROWS - 1;
    term.col = 0;
}

void terminal_clear() {
    // Reset VGA hardware cursor to 0
    unsigned short pos = 0;
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0F),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(pos&0xFF)),"Nd"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0E),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(pos>>8)),"Nd"((unsigned short)0x3D5));
    for (int r = 0; r < VGA_ROWS; r++)
        vga_clear_row(r, MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
    term.row = 0;
    term.col = 0;
}

void terminal_init() {
    serial_init();
    // Hard wipe entire VGA buffer
    unsigned short* v = (unsigned short*)0xB8000;
    for (int i = 0; i < 80*25; i++) v[i] = 0x0720;
    term.col = 0;
    term.row = 0;
    term.color = MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK);
    term.cmd_len = 0;
    term.ai_mode = 0;
    term.color   = MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK);
    term.cmd_len = 0;
    term.ai_mode = 0;

    // Banner
    terminal_set_color(MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
    terminal_print("=====================================\n");
    terminal_print("   AIOS  -  AI Operating System      \n");
    terminal_print("   All commands processed by AI      \n");
    terminal_print("=====================================\n");
    terminal_set_color(MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
    terminal_print("Memory manager  : OK\n");
    terminal_print("Terminal driver : OK\n");
    terminal_print("AI interface    : READY\n");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print("\n");
}

void terminal_set_color(unsigned char color) {
    term.color = color;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    if (c == '\r') return;

    vga_set(term.row, term.col, c, term.color);
    term.col++;
    if (term.col >= VGA_COLS) {
        term.col = 0;
        term.row++;
        if (term.row >= VGA_ROWS) terminal_scroll();
    }
}

void terminal_newline() {
    term.col = 0;
    term.row++;
    if (term.row >= VGA_ROWS) terminal_scroll();
}

void terminal_print(const char* s) {
    if(vga_active) vga_shell_print(s, 10);
    while (*s) terminal_putchar(*s++);
}

void terminal_print_color(const char* s, unsigned char color) {
    unsigned char old = term.color;
    term.color = color;
    terminal_print(s);
    term.color = old;
}

void terminal_print_int(int n) {
    if (n < 0) { terminal_putchar('-'); n = -n; }
    if (n == 0) { terminal_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) terminal_putchar(buf[j]);
}

void terminal_print_hex(unsigned int n) {
    terminal_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        int nibble = (n >> i) & 0xF;
        terminal_putchar(nibble < 10 ? '0'+nibble : 'A'+nibble-10);
    }
}

void terminal_reset_input() {
    term.cmd_len = 0;
}
void terminal_render_prompt() {
    terminal_set_color(MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
    terminal_print("AIOS");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print("> ");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    if(vga_active) vga_shell_prompt();
}

void terminal_handle_key(char c) {
    if (term.ai_mode) return;  // AI is thinking, ignore input

    if (c == '\n' || c == '\r') {
        // Submit command to AI processor
        terminal_newline();
        term.cmd_buf[term.cmd_len] = 0;

        if (term.cmd_len > 0) {
            ai_process_input(term.cmd_buf);
        } else {
            terminal_render_prompt();
        }

        // Reset command buffer
        term.cmd_len = 0;
        return;
    }

    if (c == '\b') {
        // Backspace
        if (term.cmd_len > 0) {
            term.cmd_len--;
            if (term.col > 0) term.col--;
            vga_set(term.row, term.col, ' ', term.color);
        }
        return;
    }

    // Regular character
    if (term.cmd_len < CMD_BUF_MAX - 1) {
        term.cmd_buf[term.cmd_len++] = c;
        terminal_putchar(c);
        if(vga_active){ char s[2]; s[0]=c; s[1]=0; vga_shell_print(s, 14); }
    }
}
