import os

BASE = os.getcwd()

def write(path, content):
    full = os.path.join(BASE, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(content)
    print("  [OK] " + path)

def patch(path, old, new, required=True):
    full = os.path.join(BASE, path)
    with open(full, 'r') as f:
        c = f.read()
    if old not in c:
        if required:
            print("  [WARN] not found in " + path)
        return
    with open(full, 'w') as f:
        f.write(c.replace(old, new, 1))
    print("  [OK] patched " + path)

def add_include(path, line):
    full = os.path.join(BASE, path)
    with open(full, 'r') as f:
        c = f.read()
    if line in c:
        print("  [OK] include exists in " + path)
        return
    lines = c.split('\n')
    last = 0
    for i, l in enumerate(lines):
        if l.strip().startswith('#include'):
            last = i
    lines.insert(last + 1, line)
    with open(full, 'w') as f:
        f.write('\n'.join(lines))
    print("  [OK] added include to " + path)

HEADER = '#ifndef AIOS_UI_H\n#define AIOS_UI_H\nvoid aios_ui_init(void);\nvoid aios_ui_draw(void);\nvoid aios_ui_tick(void);\nvoid aios_ui_set_status(const char* s);\nvoid aios_ui_print(const char* t, unsigned char c);\nvoid aios_ui_prompt(void);\nvoid aios_ui_input_char(char c);\nvoid aios_ui_input_backspace(void);\nvoid aios_ui_input_clear(void);\nconst char* aios_ui_get_input(void);\nextern int aios_ui_active;\n#endif\n'

UI_C = open('kernel/graphics/aios_ui.c').read() if os.path.exists('kernel/graphics/aios_ui.c') else ''

print("\n=== AIMERANCIA Installer ===\n")

print("[1] Writing header...")
write("kernel/graphics/aios_ui.h", HEADER)

print("\n[2] Patching kernel_main.c...")
add_include("kernel/kernel_main.c", '#include "graphics/aios_ui.h"')

patch("kernel/kernel_main.c",
    "        vga_shell_init();",
    "        aios_ui_init();")

patch("kernel/kernel_main.c",
    "            autonomy_auto_tick((int)timer_ticks_bss);",
    "            autonomy_auto_tick((int)timer_ticks_bss);\n            if(aios_ui_active) aios_ui_tick();")

patch("kernel/kernel_main.c",
    "        if(sc==0x0E||sc==0x53){terminal_handle_key('\\b');continue;}",
    "        if(sc==0x0E||sc==0x53){if(aios_ui_active)aios_ui_input_backspace();else terminal_handle_key('\\b');continue;}")

patch("kernel/kernel_main.c",
    "        if(sc==0x1C){terminal_handle_key('\\n');continue;}",
    "        if(sc==0x1C){if(aios_ui_active){static char cb[48];const char*s=aios_ui_get_input();int i=0;while(s[i]&&i<47){cb[i]=s[i];i++;}cb[i]=0;aios_ui_input_clear();if(cb[0]){extern void ai_process_input(const char*);ai_process_input(cb);}aios_ui_prompt();}else terminal_handle_key('\\n');continue;}")

patch("kernel/kernel_main.c",
    "            if(ch) terminal_handle_key(ch);",
    "            if(ch){if(aios_ui_active)aios_ui_input_char(ch);else terminal_handle_key(ch);}")

print("\n[3] Patching terminal.c...")
add_include("kernel/terminal/terminal.c", '#include "../graphics/aios_ui.h"')
patch("kernel/terminal/terminal.c",
    "void terminal_print_color(const char* s, unsigned char color) {",
    "void terminal_print_color(const char* s, unsigned char color) {\n    if(aios_ui_active){ aios_ui_print(s, 14); return; }",
    required=False)

print("\n[4] grub.cfg...")
g = "iso/boot/grub/grub.cfg"
if os.path.exists(g):
    with open(g) as f: txt = f.read()
    with open(g, 'w') as f: f.write(txt.replace("AIOS", "AIMERANCIA"))
    print("  [OK]")

print("\nDone! Now run:  make 2>&1 | grep -v 'warning|NOTE'")
