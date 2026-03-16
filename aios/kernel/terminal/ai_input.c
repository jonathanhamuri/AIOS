#include "ai_input.h"
#include "../graphics/vga.h"
#include "terminal.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../process/process.h"
#include "../compiler/compiler.h"
#include "../ai/ai_exec.h"

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



static void cmd_compile(const char* src) {
    if (!src || !src[0]) {
        terminal_print_color("Usage: compile <source>\n", MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));
        terminal_print("Example: compile print \"Hello from compiled code!\";\n");
        return;
    }
    terminal_print_color("[COMPILER] Compiling...\n", MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));

    compile_result_t result;
    int r = compiler_compile(src, &result);

    if (r != COMPILE_OK) {
        terminal_print_color("[COMPILER] Error: ", MAKE_COLOR(COLOR_BRED, COLOR_BLACK));
        terminal_print(result.errmsg);
        terminal_newline();
        return;
    }

    terminal_print_color("[COMPILER] OK - ", MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
    terminal_print_int(result.size);
    terminal_print_color(" bytes generated\n", MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
    terminal_print_color("[COMPILER] Running...\n", MAKE_COLOR(COLOR_BYELLOW, COLOR_BLACK));

    // Execute compiled binary
    process_exec_binary("compiled", result.code, result.size);
    kfree(result.code);

    terminal_print_color("[COMPILER] Done\n", MAKE_COLOR(COLOR_BGREEN, COLOR_BLACK));
}

static void cmd_ps(const char* args) {
    process_list();
}

static void cmd_run(const char* args) {
    terminal_print_color("[AI] run command - process loader ready\n",
                         MAKE_COLOR(COLOR_BCYAN, COLOR_BLACK));
    terminal_print("Usage: compiler will generate binaries to run here\n");
}

// ── Phase 4 AI natural language engine ───────────────────────────────
static void ai_natural_language(const char* input) {
    ai_exec(input);
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
    } else if (str_starts(input, "compile ")) {
        cmd_compile(input + 8);
    } else if (str_eq(input, "ps")) {
        cmd_ps(0);
    } else if (str_starts(input, "run ")) {
        cmd_run(input + 4);
    } else if (str_starts(input, "echo ")) {
        cmd_echo(input + 5);
    } else {
        // Everything else goes to AI natural language processor
        ai_natural_language(input);
    }

    terminal_print("\n");
    terminal_render_prompt();
}
