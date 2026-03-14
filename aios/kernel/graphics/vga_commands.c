#include "vga.h"
#include "vga_commands.h"
#include "../terminal/terminal.h"

static int streq(const char* a, const char* b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int starts(const char* s, const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}

// Parse color name to VGA palette index
static unsigned char parse_color(const char* s){
    if(starts(s,"red"))    return 4;
    if(starts(s,"green"))  return 2;
    if(starts(s,"blue"))   return 1;
    if(starts(s,"yellow")) return 14;
    if(starts(s,"cyan"))   return 3;
    if(starts(s,"white"))  return 15;
    if(starts(s,"magenta"))return 5;
    if(starts(s,"orange")) return 6;
    return 10; // default green
}

// Draw a filled circle
static void draw_circle(int cx, int cy, int r, unsigned char color){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r)
                vga_putpixel(cx+x,cy+y,color);
}

// Draw a bar chart from values
static void draw_bars(int* vals, int count, unsigned char color){
    vga_rectfill(0,14,320,176,0);
    int bw = 300/count;
    for(int i=0;i<count;i++){
        int h = vals[i];
        if(h>160) h=160;
        int x = 10+i*bw;
        int y = 175-h;
        vga_rectfill(x,y,bw-2,h,color);
        vga_drawint(x,177,vals[i],14,0);
    }
}

// Simple atoi
static int my_atoi(const char* s){
    int n=0;
    while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;}
    return n;
}

// Skip past next space
static const char* skip_word(const char* s){
    while(*s&&*s!=' ')s++;
    while(*s==' ')s++;
    return s;
}

int vga_handle_command(const char* input){
    if(!vga_active) return 0;

    // draw circle <color>
    if(starts(input,"draw circle")){
        const char* rest = skip_word(skip_word(input));
        unsigned char col = parse_color(rest);
        vga_rectfill(0,14,320,176,0);
        draw_circle(160,100,50,col);
        vga_drawstring(100,170,"Circle drawn!",10,0);
        terminal_print_color("[VGA] Circle drawn\n",3<<4|10);
        return 1;
    }

    // draw rect <color>
    if(starts(input,"draw rect")){
        const char* rest = skip_word(skip_word(input));
        unsigned char col = parse_color(rest);
        vga_rectfill(0,14,320,176,0);
        vga_rectfill(80,50,160,100,col);
        vga_rect(80,50,160,100,15);
        vga_drawstring(110,170,"Rectangle drawn!",10,0);
        terminal_print_color("[VGA] Rectangle drawn\n",3<<4|10);
        return 1;
    }

    // draw line <color>
    if(starts(input,"draw line")){
        const char* rest = skip_word(skip_word(input));
        unsigned char col = parse_color(rest);
        vga_rectfill(0,14,320,176,0);
        vga_line(10,180,310,20,col);
        vga_line(10,20,310,180,col);
        vga_drawstring(110,170,"Lines drawn!",10,0);
        terminal_print_color("[VGA] Lines drawn\n",3<<4|10);
        return 1;
    }

    // show memory
    if(starts(input,"show memory")||starts(input,"visualize memory")){
        extern int pmm_free_pages();
        int free = pmm_free_pages();
        int used = 1024 - free;
        if(used<0) used=0;
        int vals[4] = {free*100/1024, used*100/1024, free/10, used/10};
        vga_rectfill(0,14,320,176,0);
        draw_bars(vals,4,2);
        vga_drawstring(2,20,"Free% Used% FreeK UsedK",11,0);
        terminal_print_color("[VGA] Memory chart shown\n",3<<4|10);
        return 1;
    }

    // show ticks / visualize ticks
    if(starts(input,"show ticks")||starts(input,"visualize ticks")){
        extern unsigned int timer_ticks_bss;
        int t = (int)timer_ticks_bss;
        vga_rectfill(0,14,320,176,0);
        vga_drawstring(60,80,"Timer Ticks:",11,0);
        vga_drawint(180,80,t,14,0);
        int bar = t % 280;
        vga_rectfill(20,120,bar,20,14);
        vga_rect(20,120,280,20,15);
        terminal_print_color("[VGA] Ticks shown\n",3<<4|10);
        return 1;
    }

    // clear screen
    if(streq(input,"clear")){
        vga_shell_init();
        return 1;
    }

    // draw star
    if(starts(input,"draw star")){
        const char* rest = skip_word(skip_word(input));
        unsigned char col = parse_color(rest);
        vga_rectfill(0,14,320,176,0);
        int cx=160,cy=100,r=60;
        // 5 point star using lines
        for(int i=0;i<5;i++){
            int a1=(i*72-90)*314/18000;
            int a2=((i*72+144)-90)*314/18000;
            int x1=cx+r*(a1)/100;
            int y1=cy+r*(a1+50)/100;
            int x2=cx+r*(a2)/100;
            int y2=cy+r*(a2+50)/100;
            vga_line(x1,y1,x2,y2,col);
        }
        // simpler: just draw lines from center
        vga_line(cx,cy-r,cx+r/2,cy+r/2,col);
        vga_line(cx+r/2,cy+r/2,cx-r,cy,col);
        vga_line(cx-r,cy,cx+r,cy,col);
        vga_line(cx+r,cy,cx-r/2,cy+r/2,col);
        vga_line(cx-r/2,cy+r/2,cx,cy-r,col);
        vga_drawstring(120,170,"Star drawn!",col,0);
        terminal_print_color("[VGA] Star drawn\n",3<<4|10);
        return 1;
    }

    // paint / fill screen <color>
    if(starts(input,"paint ")||starts(input,"fill screen")){
        const char* rest = starts(input,"paint ") ? input+6 : skip_word(skip_word(input));
        unsigned char col = parse_color(rest);
        vga_rectfill(0,14,320,176,col);
        vga_drawstring(100,100,"Screen filled!",0,col);
        terminal_print_color("[VGA] Screen painted\n",3<<4|10);
        return 1;
    }

    return 0; // not a visual command
}
