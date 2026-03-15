#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

// Multiboot info structure (relevant parts)
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned char syms[16];
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int drives_length;
    unsigned int drives_addr;
    unsigned int config_table;
    unsigned int boot_loader_name;
    unsigned int apm_table;
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
    unsigned long long framebuffer_addr;
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bpp;
    unsigned char framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

// Framebuffer info
extern unsigned int fb_addr;
extern unsigned int fb_width;
extern unsigned int fb_height;
extern unsigned int fb_pitch;
extern unsigned char fb_bpp;
extern unsigned char fb_active;

int  fb_init();
void fb_putpixel(int x, int y, unsigned int color);
void fb_clear(unsigned int color);
void fb_rectfill(int x, int y, int w, int h, unsigned int color);
void fb_drawchar(int x, int y, char c, unsigned int fg, unsigned int bg);
void fb_drawstring(int x, int y, const char* s, unsigned int fg, unsigned int bg);
void fb_drawint(int x, int y, int val, unsigned int fg, unsigned int bg);
void fb_shell_init();
void fb_shell_print(const char* s, unsigned int color);
void fb_shell_prompt();
void fb_shell_newline();
void fb_scroll_up();
void fb_scroll_down();

// Colors (32bpp ARGB)
#define FB_BLACK    0x00000000
#define FB_WHITE    0x00FFFFFF
#define FB_GREEN    0x0000FF00
#define FB_CYAN     0x0000FFFF
#define FB_RED      0x00FF0000
#define FB_YELLOW   0x00FFFF00
#define FB_PURPLE   0x00800080
#define FB_DGRAY    0x00222222
#define FB_LGRAY    0x00AAAAAA
#define FB_BLUE     0x000000FF
#define FB_ORANGE   0x00FF8800

#endif
