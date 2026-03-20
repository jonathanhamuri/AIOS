#include "aimerancia_ui.h"
#include "vga.h"
#include "../keyboard/keyboard.h"
#include "../timer/timer.h"

// ======================================================
// SCREEN CONFIG
// ======================================================
#define SW 640
#define SH 480
#define PANEL_W 120

// ======================================================
// COLORS
// ======================================================
#define C_BG     0
#define C_MAIN   14
#define C_ACCENT 3

// ======================================================
// INPUT
// ======================================================
#define INPUT_MAX 128
static char input[INPUT_MAX];
static int input_len = 0;

// ======================================================
// LOG
// ======================================================
#define LOG_LINES 18
#define LOG_COLS  52
static char logbuf[LOG_LINES][LOG_COLS];
static int log_line = 0;

// ======================================================
// UTILS
// ======================================================
static void log_push(const char* s) {
    int i = 0;
    while (s[i] && i < LOG_COLS - 1) {
        logbuf[log_line][i] = s[i];
        i++;
    }
    logbuf[log_line][i] = 0;
    log_line = (log_line + 1) % LOG_LINES;
}

// ======================================================
// BOOT SPLASH
// ======================================================
static void boot_splash() {
    vga_clear(C_BG);
    vga_draw_text(200, 220, "AIMERANCIA", C_MAIN, C_BG);
    for (int i = 0; i < 20000000; i++) asm volatile("nop");
    vga_clear(C_BG);
}

// ======================================================
// FAKE VOICE WAVE
// ======================================================
static void draw_wave() {
    int base = 400;
    for (int x = 0; x < 100; x++) {
        int h = (x * 13 + timer_ticks()) % 20;
        vga_draw_rect(20 + x * 2, base - h, 1, h, C_ACCENT);
    }
}

// ======================================================
// COMMAND PARSER
// ======================================================
static void exec_command() {
    if (input_len == 0) return;

    if (!strcmp(input, "help")) {
        log_push("Commands: help clear hello");
    } else if (!strcmp(input, "clear")) {
        for (int i = 0; i < LOG_LINES; i++)
            logbuf[i][0] = 0;
    } else if (!strcmp(input, "hello")) {
        log_push("Hello, I am AIMERANCIA.");
    } else {
        log_push("Unknown command");
    }

    input_len = 0;
    input[0] = 0;
}

// ======================================================
// INPUT HANDLER
// ======================================================
void aimerancia_ui_on_key(char c) {
    if (c == '\n') {
        exec_command();
        return;
    }
    if (c == '\b') {
        if (input_len > 0) input[--input_len] = 0;
        return;
    }
    if (input_len < INPUT_MAX - 1) {
        input[input_len++] = c;
        input[input_len] = 0;
    }
}

// ======================================================
// INIT
// ======================================================
void aimerancia_ui_init() {
    boot_splash();
    log_push("AIMERANCIA ONLINE");
    log_push("Type 'help'");
}

// ======================================================
// RENDER LOOP
// ======================================================
void aimerancia_ui_update() {
    vga_clear(C_BG);

    // SIDE PANEL
    vga_draw_rect(0, 0, PANEL_W, SH, 1);
    vga_draw_text(10, 10, "AIMERANCIA", C_MAIN, 1);

    // LOG
    for (int i = 0; i < LOG_LINES; i++) {
        int idx = (log_line + i) % LOG_LINES;
        vga_draw_text(PANEL_W + 10, 20 + i * 18, logbuf[idx], C_MAIN, C_BG);
    }

    // INPUT BAR
    vga_draw_rect(PANEL_W, SH - 30, SW - PANEL_W, 30, 1);
    vga_draw_text(PANEL_W + 10, SH - 22, input, C_MAIN, 1);

    draw_wave();
}
