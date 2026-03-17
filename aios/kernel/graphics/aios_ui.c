#include "aios_ui.h"
#include "vga.h"
#include "../terminal/terminal.h"
#include "../mm/heap.h"

int aios_ui_active = 0;

// Layout constants for 320x200
#define SCREEN_W    320
#define SCREEN_H    200

// Left panel
#define PANEL_X     0
#define PANEL_W     80
#define PANEL_H     200

// Main area
#define MAIN_X      80
#define MAIN_W      240
#define MAIN_H      200

// Orb position (center of main area)
#define ORB_X       190
#define ORB_Y       115
#define ORB_R       25

// Output area
#define OUT_X       82
#define OUT_Y       12
#define OUT_W       236
#define OUT_H       80

// Input bar
#define INPUT_Y     183
#define INPUT_H     17

// Status bar
#define STATUS_Y    0
#define STATUS_H    9

// Colors
#define COL_BG          0    // black
#define COL_PANEL_BG    8    // dark gray
#define COL_CYAN        3    // cyan
#define COL_GREEN       2    // green
#define COL_YELLOW      14   // yellow
#define COL_WHITE       15   // white
#define COL_BLUE        1    // blue
#define COL_RED         4    // red
#define COL_MAGENTA     5    // magenta
#define COL_DGRAY       8    // dark gray

// Animation state
static int orb_pulse = 0;
static int orb_dir = 1;
static int particle_angle = 0;
static int tick_count = 0;
static int orb_state = 0; // 0=idle, 1=listening, 2=thinking

// Output buffer
#define OUT_LINES   12
#define OUT_COLS    29
static char out_buf[OUT_LINES][OUT_COLS+1];
static unsigned char out_col[OUT_LINES];
static int out_line = 0;
static int out_cx = 0;

// Input buffer
#define INPUT_MAX   40
static char input_buf[INPUT_MAX+1];
static int input_len = 0;

// Status text
static char status_text[32] = "Listening...";
static char aios_name[] = "A.I.O.S.";

static const char* status_str = "Online";

// Simple pseudo-random for particles
static int prng_state = 12345;
static int prng(){
    prng_state = prng_state * 1103515245 + 12345;
    return (prng_state>>16) & 0x7FFF;
}

// Draw a filled circle using VGA
static void draw_circle_fill(int cx, int cy, int r, unsigned char color){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r)
                vga_putpixel(cx+x,cy+y,color);
}

// Draw circle outline
static void draw_circle_outline(int cx, int cy, int r, unsigned char color){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++){
            int d = x*x+y*y;
            if(d>=r*r-r && d<=r*r+r)
                vga_putpixel(cx+x,cy+y,color);
        }
}

// Draw left stats panel
static void draw_panel(){
    // Panel background
    vga_rectfill(PANEL_X, 0, PANEL_W, SCREEN_H, COL_PANEL_BG);
    vga_line(PANEL_W-1, 0, PANEL_W-1, SCREEN_H-1, COL_CYAN);

    // AIOS name at top
    vga_drawstring(2, 2, "A.I.O.S.", COL_CYAN, COL_PANEL_BG);
    // Online indicator
    vga_rectfill(60, 3, 5, 5, COL_GREEN);
    vga_drawstring(66, 2, "ON", COL_GREEN, COL_PANEL_BG);

    // Separator
    vga_line(2, 11, PANEL_W-3, 11, COL_CYAN);

    // System Stats box
    vga_drawstring(2, 13, "SYS STATS", COL_YELLOW, COL_PANEL_BG);
    vga_line(2, 21, PANEL_W-3, 21, COL_DGRAY);

    // Memory bar
    extern int pmm_free_pages();
    int free_p = pmm_free_pages();
    int total_p = 256;
    int used_p = total_p - free_p;
    if(used_p < 0) used_p = 0;
    int mem_pct = used_p * 100 / total_p;
    int mem_bar = mem_pct * 70 / 100;

    vga_drawstring(2, 23, "MEM", COL_CYAN, COL_PANEL_BG);
    vga_rectfill(2, 31, 70, 5, COL_DGRAY);
    vga_rectfill(2, 31, mem_bar, 5, mem_pct>80?COL_RED:COL_CYAN);
    vga_drawstring(2, 37, " % used", COL_WHITE, COL_PANEL_BG);
    // print mem_pct
    char pbuf[4];
    pbuf[0]='0'+mem_pct/10; pbuf[1]='0'+mem_pct%10; pbuf[2]='%'; pbuf[3]=0;
    vga_drawstring(2, 37, pbuf, COL_WHITE, COL_PANEL_BG);

    // Ticks counter
    extern unsigned int timer_ticks_bss;
    int ticks = (int)timer_ticks_bss;
    vga_drawstring(2, 47, "TICKS", COL_CYAN, COL_PANEL_BG);
    vga_drawint(40, 47, ticks, COL_WHITE, COL_PANEL_BG);

    // Uptime
    int uptime_s = ticks / 100;
    int uptime_m = uptime_s / 60;
    vga_drawstring(2, 57, "UP ", COL_CYAN, COL_PANEL_BG);
    vga_drawint(24, 57, uptime_m, COL_WHITE, COL_PANEL_BG);
    vga_drawstring(40, 57, "min", COL_DGRAY, COL_PANEL_BG);

    // Separator
    vga_line(2, 67, PANEL_W-3, 67, COL_CYAN);

    // Network box
    vga_drawstring(2, 69, "NETWORK", COL_YELLOW, COL_PANEL_BG);
    vga_line(2, 77, PANEL_W-3, 77, COL_DGRAY);
    vga_drawstring(2, 79, "IP", COL_CYAN, COL_PANEL_BG);
    vga_drawstring(18, 79, "10.0.2.15", COL_WHITE, COL_PANEL_BG);
    vga_drawstring(2, 89, "NET", COL_CYAN, COL_PANEL_BG);
    vga_rectfill(20, 90, 6, 6, COL_GREEN);

    // Separator
    vga_line(2, 100, PANEL_W-3, 100, COL_CYAN);

    // Skills box
    vga_drawstring(2, 102, "SKILLS", COL_YELLOW, COL_PANEL_BG);
    vga_line(2, 110, PANEL_W-3, 110, COL_DGRAY);
    extern int learning_count;
    vga_drawstring(2, 112, "LEARNED", COL_CYAN, COL_PANEL_BG);
    vga_drawint(58, 112, learning_count, COL_WHITE, COL_PANEL_BG);

    // Separator
    vga_line(2, 122, PANEL_W-3, 122, COL_CYAN);

    // Commands box
    vga_drawstring(2, 124, "SESSION", COL_YELLOW, COL_PANEL_BG);
    vga_line(2, 132, PANEL_W-3, 132, COL_DGRAY);
    vga_drawstring(2, 134, "CMDS", COL_CYAN, COL_PANEL_BG);
    vga_drawstring(2, 144, "PROACTIVE", COL_CYAN, COL_PANEL_BG);
    vga_rectfill(60, 145, 6, 6, COL_GREEN);

    // Bottom version
    vga_line(2, 188, PANEL_W-3, 188, COL_CYAN);
    vga_drawstring(2, 190, "v1.0", COL_DGRAY, COL_PANEL_BG);
    vga_drawstring(30, 190, "AIOS", COL_CYAN, COL_PANEL_BG);
}

// Draw the animated orb
static void draw_orb(){
    // Clear orb area
    vga_rectfill(MAIN_X, STATUS_H+1, MAIN_W, INPUT_Y-STATUS_H-2, COL_BG);

    // Outer rings (animated)
    int r1 = ORB_R + 15 + orb_pulse/4;
    int r2 = ORB_R + 10 + orb_pulse/3;
    int r3 = ORB_R + 5  + orb_pulse/2;

    // Draw rings with varying intensity
    draw_circle_outline(ORB_X, ORB_Y, r1, COL_DGRAY);
    draw_circle_outline(ORB_X, ORB_Y, r2,
        orb_state==1 ? COL_CYAN : COL_BLUE);
    draw_circle_outline(ORB_X, ORB_Y, r3,
        orb_state==1 ? COL_CYAN : COL_BLUE);

    // Particles orbiting
    for(int p=0;p<8;p++){
        int angle = (particle_angle + p*45) % 360;
        // Simple trig approximation
        int r_orbit = ORB_R + 12;
        int px, py;
        // Use lookup-style computation
        int a = angle % 90;
        int q = angle / 90;
        int ca, sa;
        // cos/sin approximation (scaled by 100)
        if(a < 30){ ca=87-a; sa=a*2; }
        else if(a < 60){ ca=60-a; sa=87-(a-30); }
        else { ca=a-60; sa=87-(90-a); }
        if(q==0){px= ca; py= sa;}
        else if(q==1){px=-sa; py= ca;}
        else if(q==2){px=-ca; py=-sa;}
        else{px= sa; py=-ca;}
        px = ORB_X + px*r_orbit/100;
        py = ORB_Y + py*r_orbit/100;
        vga_putpixel(px, py,
            (p%2==0) ? COL_CYAN : COL_WHITE);
    }

    // Core glow layers
    draw_circle_fill(ORB_X, ORB_Y, ORB_R-2,   COL_BLUE);
    draw_circle_fill(ORB_X, ORB_Y, ORB_R-8,   COL_CYAN);
    draw_circle_fill(ORB_X, ORB_Y, ORB_R-16,  COL_WHITE);
    draw_circle_fill(ORB_X, ORB_Y, ORB_R-20,  COL_CYAN);

    // Pulse effect on core
    if(orb_pulse > 4){
        draw_circle_fill(ORB_X, ORB_Y, ORB_R-18, COL_WHITE);
    }

    // Name below orb
    vga_drawstring(ORB_X-28, ORB_Y+ORB_R+5,
        "A.I.O.S.", COL_CYAN, COL_BG);

    // Status below name
    vga_drawstring(ORB_X-24, ORB_Y+ORB_R+14,
        status_text, COL_GREEN, COL_BG);

    // Output text - right side, not overlapping orb
    int start_line = out_line - 10;
    if(start_line < 0) start_line += OUT_LINES;
    for(int i=0;i<10;i++){
        int idx = (start_line+i) % OUT_LINES;
        // Clear line first
        vga_rectfill(MAIN_X+2, OUT_Y+i*10, MAIN_W-4, 9, COL_BG);
        if(out_buf[idx][0]){
            vga_drawstring(MAIN_X+2, OUT_Y+i*10,
                out_buf[idx], out_col[idx], COL_BG);
        }
    }
}

// Draw status bar at top
static void draw_status_bar(){
    vga_rectfill(MAIN_X, 0, MAIN_W, STATUS_H, COL_PANEL_BG);
    vga_line(MAIN_X, STATUS_H, SCREEN_W-1, STATUS_H, COL_CYAN);

    // Time (simulated from ticks)
    extern unsigned int timer_ticks_bss;
    int t = (int)timer_ticks_bss / 100;
    int sec = t % 60;
    int min = (t/60) % 60;
    int hr  = (t/3600) % 24;

    char timebuf[12];
    timebuf[0]='0'+hr/10; timebuf[1]='0'+hr%10;
    timebuf[2]=':';
    timebuf[3]='0'+min/10; timebuf[4]='0'+min%10;
    timebuf[5]=':';
    timebuf[6]='0'+sec/10; timebuf[7]='0'+sec%10;
    timebuf[8]=0;

    vga_drawstring(170, 1, timebuf, COL_CYAN, COL_PANEL_BG);
    vga_drawstring(240, 1, "AIOS", COL_YELLOW, COL_PANEL_BG);
}

// Draw input bar
static void draw_input_bar(){
    vga_rectfill(MAIN_X, INPUT_Y, MAIN_W, INPUT_H, COL_PANEL_BG);
    vga_line(MAIN_X, INPUT_Y, SCREEN_W-1, INPUT_Y, COL_GREEN);

    // Prompt
    vga_drawstring(MAIN_X+2, INPUT_Y+4, "aios", COL_GREEN, COL_PANEL_BG);
    vga_drawstring(MAIN_X+34, INPUT_Y+4, "@", COL_WHITE, COL_PANEL_BG);
    vga_drawstring(MAIN_X+42, INPUT_Y+4, "system", COL_CYAN, COL_PANEL_BG);
    vga_drawstring(MAIN_X+90, INPUT_Y+4, ":~$", COL_WHITE, COL_PANEL_BG);

    // Input text (show last 18 chars)
    int start = input_len > 18 ? input_len-18 : 0;
    int cx = MAIN_X+114;
    for(int i=start;i<input_len;i++){
        vga_drawchar(cx, INPUT_Y+4, input_buf[i], COL_WHITE, COL_PANEL_BG);
        cx+=8;
    }
    // Cursor
    if(tick_count%20 < 10)
        vga_rectfill(cx, INPUT_Y+3, 6, 9, COL_WHITE);
}

void aios_ui_init(){
    aios_ui_active = 1;
    orb_pulse = 0;
    orb_dir = 1;
    particle_angle = 0;
    tick_count = 0;
    input_len = 0;
    out_line = 0;

    // Clear output buffer
    for(int i=0;i<OUT_LINES;i++){
        out_buf[i][0]=0;
        out_col[i]=COL_WHITE;
    }

    // Full redraw
    vga_clear(COL_BG);
    draw_panel();
    draw_orb();
    draw_status_bar();
    draw_input_bar();

    aios_ui_print("A.I.O.S. Ready.", COL_CYAN);
    aios_ui_print("All systems nominal.", COL_GREEN);
}

void aios_ui_tick(){
    if(!aios_ui_active) return;
    tick_count++;

    // Animate orb pulse
    orb_pulse += orb_dir;
    if(orb_pulse >= 8) orb_dir = -1;
    if(orb_pulse <= 0) orb_dir = 1;

    // Rotate particles
    if(tick_count % 3 == 0)
        particle_angle = (particle_angle + 5) % 360;

    // Redraw animated elements every tick
    if(tick_count % 2 == 0){
        draw_orb();
        draw_status_bar();
        draw_input_bar();
        // Redraw panel stats periodically
        if(tick_count % 20 == 0)
            draw_panel();
    }
}

void aios_ui_set_status(const char* s){
    int i=0;
    while(s[i]&&i<31){status_text[i]=s[i];i++;}
    status_text[i]=0;
}

void aios_ui_print(const char* text, unsigned char color){
    // Add to output buffer
    int i=0;
    while(text[i]&&i<OUT_COLS){
        out_buf[out_line][i]=text[i];i++;
    }
    out_buf[out_line][i]=0;
    out_col[out_line]=color;
    out_line=(out_line+1)%OUT_LINES;
    // Clear next line
    out_buf[out_line][0]=0;
    draw_orb();
}

void aios_ui_prompt(){
    aios_ui_set_status("Listening...");
    input_len=0;
    input_buf[0]=0;
    draw_input_bar();
}

void aios_ui_input_char(char c){
    if(input_len<INPUT_MAX){
        input_buf[input_len++]=c;
        input_buf[input_len]=0;
    }
    draw_input_bar();
}

void aios_ui_input_backspace(){
    if(input_len>0){
        input_len--;
        input_buf[input_len]=0;
    }
    draw_input_bar();
}

void aios_ui_input_clear(){
    input_len=0;
    input_buf[0]=0;
    draw_input_bar();
}

void aios_ui_draw(){
    vga_clear(COL_BG);
    draw_panel();
    draw_orb();
    draw_status_bar();
    draw_input_bar();
}
