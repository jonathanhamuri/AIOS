#include "ai_input.h"
#include "terminal.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"

// ── string helpers ────────────────────────────────────────────────────
static int str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static int str_starts(const char* s, const char* prefix) {
    while (*prefix) { if (*s != *prefix) return 0; s++; prefix++; }
    return 1;
}

static int str_len(const char* s) {
    int n = 0; while (*s++) n++; return n;
}

// ── built-in command handlers ─────────────────────────────────────────
static void cmd_help(const char* args) {
    terminal_set_color(MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
    terminal_print("AIOS built-in commands:\n");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print("  help       - show this message\n");
    terminal_print("  clear      - clear the screen\n");
    terminal_print("  mem        - show memory status\n");
    terminal_print("  echo <msg> - print a message\n");
    terminal_print("  about      - about AIOS\n");
    terminal_set_color(MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
    terminal_print("  Any other input -> AI language processor (Phase 4)\n");
}

static void cmd_clear(const char* args) {
    terminal_clear();
    terminal_render_prompt();
    return;
}

static void cmd_mem(const char* args) {
    terminal_set_color(MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
    terminal_print("Memory status:\n");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print("  Free pages : ");
    terminal_print_int(pmm_free_pages());
    terminal_print(" x 4KB = ");
    terminal_print_int(pmm_free_pages() * 4);
    terminal_print(" KB free\n");
}

static void cmd_echo(const char* args) {
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    terminal_print(args);
    terminal_print("\n");
}

static void cmd_about(const char* args) {
    terminal_set_color(MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
    terminal_print("============================================\n");
    terminal_print("  AIOS - Artificial Intelligence OS        \n");
    terminal_print("  Built from scratch:                      \n");
    terminal_print("  Layer 1: x86 Assembly (hardware control) \n");
    terminal_print("  Layer 2: C Kernel (memory, terminal)     \n");
    terminal_print("  Layer 3: Compiler (bridges C to kernel)  \n");
    terminal_print("  Layer 4: AI language (plain English/FR)  \n");
    terminal_print("  Layer 5: AIOS (AI-native OS)             \n");
    terminal_print("============================================\n");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
}

// ── AI natural language stub ──────────────────────────────────────────
// This is where Phase 4 plugs in — for now we pattern-match keywords
static void ai_natural_language(const char* input) {
    terminal_set_color(MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
    terminal_print("[AI] Processing: '");
    terminal_print(input);
    terminal_print("'\n");
    terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));

    // Simple keyword detection — Phase 4 replaces this with real NLP
    if (str_starts(input, "show memory") || str_starts(input, "afficher memoire")) {
        cmd_mem(0);
    } else if (str_starts(input, "clear screen") || str_starts(input, "effacer")) {
        cmd_clear(0);
        return;
    } else if (str_starts(input, "what is") || str_starts(input, "qu'est")) {
        terminal_print("[AI] I am AIOS - an AI-driven operating system.\n");
        terminal_print("[AI] Phase 4 will add full natural language processing.\n");
    } else if (str_starts(input, "hello") || str_starts(input, "bonjour")) {
        terminal_set_color(MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
        terminal_print("[AI] Hello! AIOS is listening. Type 'help' for commands.\n");
        terminal_set_color(MAKE_COLOR(COLOR_BWHITE, COLOR_BLACK));
    } else {
        terminal_print("[AI] Unknown command. Phase 4 NLP engine not yet loaded.\n");
        terminal_print("[AI] Type 'help' for available commands.\n");
    }
}

// ── main dispatcher ───────────────────────────────────────────────────
void ai_process_input(const char* input) {
    // Skip leading spaces
    while (*input == ' ') input++;
    if (!*input) { terminal_render_prompt(); return; }

    // Check built-in commands first
    if (str_eq(input, "help")) {
        cmd_help(0);
    } else if (str_eq(input, "clear")) {
        cmd_clear(0);
        return;
    } else if (str_eq(input, "mem")) {
        cmd_mem(0);
    } else if (str_eq(input, "about")) {
        cmd_about(0);
    } else if (str_starts(input, "echo ")) {
        cmd_echo(input + 5);
    } else {
        // Everything else goes to AI natural language processor
        ai_natural_language(input);
    }

    terminal_print("\n");
    terminal_render_prompt();
}
