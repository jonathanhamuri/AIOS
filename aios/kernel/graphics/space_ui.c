
#include "space_ui.h"
#include "framebuffer.h"
#include "../terminal/terminal.h"
#include "../ai/knowledge/kb.h"
#include "../ai/documents/doc_page.h"
#include "../mm/pmm.h"

int space_mode = 0;

/* ── Trig tables (scaled x1000) ── */
static int cos_t[36] = {
    1000, 985, 940, 866, 766, 643, 500, 342, 174, 0,
    -174,-342,-500,-643,-766,-866,-940,-985,-1000,-985,
    -940,-866,-766,-643,-500,-342,-174, 0, 174, 342,
     500, 643, 766, 866, 940, 985
};
static int sin_t[36] = {
    0, 174, 342, 500, 643, 766, 866, 940, 985, 1000,
    985, 940, 866, 766, 643, 500, 342, 174, 0,-174,
   -342,-500,-643,-766,-866,-940,-985,-1000,-985,-940,
   -866,-766,-643,-500,-342,-174
};
static int isin(int a){ return sin_t[((a%360)+360)%360/10]; }
static int icos(int a){ return cos_t[((a%360)+360)%360/10]; }

/* ── Colors ── */
#define BLACK    0x00000005
#define DEEPBLUE 0x00000818
#define SUN_CORE 0x00FFFFFF
#define SUN_MID  0x00FFD700
#define SUN_OUT  0x00FF8800
#define SUN_ATM  0x00441100
#define STARFLD  0x00AAAAAA
#define DIMSTAR  0x00333333
#define GOLD     0x00B87333
#define LGOLD    0x00D4A017
#define YGOLD    0x00FFD700
#define GREEN    0x0000CC44
#define CYAN     0x0000CCCC
#define BLUE     0x000066FF
#define RED      0x00CC2222
#define PURPLE   0x00882288
#define ORANGE   0x00CC6600
#define WHITE    0x00FFFFFF
#define GRAY     0x00444444
#define DGRAY    0x00222222

/* ── Planet definitions ── */
typedef struct {
    const char* name;
    const char* label;
    int   orbit_r;    /* orbit radius */
    int   size;       /* planet radius */
    unsigned int col1, col2, col3; /* colors */
    int   angle;      /* current orbital angle */
    int   speed;      /* degrees per tick /10 */
    const char* desc;
} planet_t;

static planet_t planets[7] = {
    {"Documents", "DOCS",    160, 14,
     0x00DDAA44, 0x00AA7722, 0x00886611, 0,   8,
     "Document editor\nEssays & Reports"},
    {"Learning",  "LEARN",   230, 11,
     0x0044AA66, 0x002277AA, 0x001144AA, 60,  6,
     "Knowledge base\nSkill learning"},
    {"Network",   "NET",     300, 13,
     0x000077CC, 0x002244AA, 0x000033AA, 120, 5,
     "RTL8139 driver\nDevice discovery"},
    {"Engineering","ENG",    370, 12,
     0x00CC6600, 0x00AA4400, 0x00882200, 180, 4,
     "Railway & Bridge\nSatellite launch"},
    {"Autonomy",  "AUTO",    200, 10,
     0x00AA2288, 0x00882266, 0x00661144, 240, 7,
     "Self-repair\nSystem guardian"},
    {"Code",      "CODE",    270, 11,
     0x0000AA88, 0x00008866, 0x00006644, 300, 5,
     "AI code gen\nNative compiler"},
    {"Scheduler", "SCHED",   340, 10,
     0x00888800, 0x00666600, 0x00444400, 30,  3,
     "Task queue\nBackground jobs"},
};

/* ── State ── */
static unsigned int W, H, CX, CY;
static int tick = 0;
static int sun_pulse = 0, sun_pdir = 1;
static int sun_speaking = 0;
static int travel_target = PLANET_NONE;
static int travel_progress = 0; /* 0-100 */
static int current_planet = PLANET_NONE;
static int returning = 0;

/* Stars */
#define NSTARS 120
static unsigned short star_x[NSTARS];
static unsigned short star_y[NSTARS];
static unsigned char  star_b[NSTARS]; /* brightness 0-3 */

static void init_stars(void){
    /* Deterministic star field */
    unsigned int seed = 0xDEADBEEF;
    for(int i=0;i<NSTARS;i++){
        seed = seed*1664525u + 1013904223u;
        star_x[i] = (unsigned short)(seed % W);
        seed = seed*1664525u + 1013904223u;
        star_y[i] = (unsigned short)(seed % H);
        star_b[i] = (unsigned char)(seed % 4);
    }
}

/* ── Drawing ── */
static void px(int x,int y,unsigned int c){
    if(x<0||y<0||(unsigned)x>=W||(unsigned)y>=H)return;
    fb_putpixel(x,y,c);
}
static void hl(int x1,int x2,int y,unsigned int c){
    for(int x=x1;x<=x2;x++)px(x,y,c);
}
static void fc(int cx,int cy,int r,unsigned int c){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r)px(cx+x,cy+y,c);
}
static void ring(int cx,int cy,int r,int t,unsigned int c){
    int r2=(r+t)*(r+t);
    for(int y=-(r+t);y<=(r+t);y++)
        for(int x=-(r+t);x<=(r+t);x++){
            int d=x*x+y*y;
            if(d>=r*r&&d<=r2)px(cx+x,cy+y,c);
        }
}
static void orbit_ring(int cx,int cy,int r,unsigned int c){
    /* Draw elliptical orbit */
    for(int a=0;a<360;a+=3){
        int ox=cx+r*icos(a)/1000;
        int oy=cy+r*isin(a)/2000; /* flatten to ellipse */
        px(ox,oy,c);
    }
}

/* ── Draw sun ── */
static void draw_sun(int x,int y,int size,int speaking){
    int p = sun_pulse;
    int sp = speaking ? p*3 : 0;

    /* Outer atmosphere glow */
    fc(x,y,size+18+p, 0x00110400);
    fc(x,y,size+12+p, 0x00220800);
    fc(x,y,size+8+p+sp, SUN_ATM);

    /* Corona rays */
    for(int i=0;i<12;i++){
        int a=(i*30+tick*2)%360;
        int r1=size+6; int r2=size+14+p+(speaking?p*2:0);
        int x1=x+r1*icos(a)/1000, y1=y+r1*isin(a)/1000;
        int x2=x+r2*icos(a)/1000, y2=y+r2*isin(a)/1000;
        /* Draw ray as line of pixels */
        int dx=x2-x1, dy=y2-y1;
        int steps=r2-r1; if(steps<1)steps=1;
        for(int s=0;s<steps;s++){
            int px2=x1+dx*s/steps;
            int py2=y1+dy*s/steps;
            px(px2,py2,0x00FF6600);
        }
    }

    /* Sun layers */
    fc(x,y,size+4+p,  SUN_OUT);
    fc(x,y,size,      0x00FF9900);
    fc(x,y,size-4,    SUN_MID);
    fc(x,y,size-8,    0x00FFEE00);
    fc(x,y,size-12,   SUN_CORE);
    if(size>14) fc(x,y,size-15, WHITE);

    /* Crosshair */
    hl(x-size+4,x+size-4,y,0x44FFFFFF);
    for(int i=y-size+4;i<=y+size-4;i++) px(x,i,0x44FFFFFF);

    /* Speaking indicator */
    if(speaking){
        ring(x,y,size+20+p,2,YGOLD);
        ring(x,y,size+26+p,2,LGOLD);
    }
}

/* ── Draw planet ── */
static int dlen(const char*s){int n=0;while(*s++)n++;return n;}
static void draw_planet(int px2,int py2,planet_t*p,int highlighted){
    /* Shadow side */
    fc(px2+p->size/4, py2+p->size/4, p->size, 0x00111111);
    /* Planet body */
    fc(px2, py2, p->size, p->col1);
    fc(px2-p->size/4, py2-p->size/4, p->size*3/4, p->col2);
    fc(px2-p->size/3, py2-p->size/3, p->size/2, p->col3);

    /* Highlight */
    if(highlighted){
        ring(px2,py2,p->size+2,2,YGOLD);
        ring(px2,py2,p->size+6,1,GOLD);
    }

    /* Label */
    int lx=px2-dlen(p->label)*4;
    int ly=py2+p->size+5;
    if(ly<(int)H-10)
        fb_drawstring(lx,ly,p->label,highlighted?YGOLD:LGOLD,BLACK);
}


/* ── Draw mini-sun (corner when on planet) ── */
static void draw_mini_sun(void){
    int sz=18;
    int mx=sz+12, my=sz+12;
    /* Background circle */
    fc(mx,my,sz+8,0x00110400);
    draw_sun(mx,my,sz,sun_speaking);
    /* AIMERANCIA label */
    fb_drawstring(4,sz*2+16,"AIMERANCIA",LGOLD,BLACK);
    /* Status */
    extern char "LISTENING..."[32];
    fb_drawstring(4,sz*2+26,"LISTENING...",GREEN,BLACK);
}

/* ── Planet surface (when traveling there) ── */
static void draw_planet_surface(int pid){
    planet_t*p=&planets[pid];
    /* Background — planet color gradient */
    fb_rectfill(0,0,W,H,BLACK);
    /* Horizon glow */
    for(int i=0;i<30;i++){
        unsigned int col=((unsigned int)(i*4)<<16)|
                         ((unsigned int)(i*2)<<8)|0x00000010;
        hl(0,W-1,H-30+i,col);
    }
    /* Planet surface lines */
    for(int i=0;i<8;i++){
        hl(0,W-1,H-80+i*8,
           i%2==0?p->col1:p->col2);
    }
    /* Mini sun top-left */
    draw_mini_sun();
    /* Planet name large */
    fb_drawstring(W/2-dlen(p->name)*4,40,p->name,YGOLD,BLACK);
    hl(W/2-60,W/2+60,52,GOLD);
}

/* ── Full solar system view ── */
static void draw_solar_system(void){
    /* Space background */
    fb_rectfill(0,0,W,H,BLACK);

    /* Stars */
    for(int i=0;i<NSTARS;i++){
        unsigned int sc=(star_b[i]==0)?DIMSTAR:
                        (star_b[i]==1)?0x00555555:
                        (star_b[i]==2)?0x00888888:STARFLD;
        /* Twinkle */
        if((tick+i*7)%40<2) sc=WHITE;
        px(star_x[i],star_y[i],sc);
    }

    /* Orbit rings */
    for(int i=0;i<7;i++){
        orbit_ring(CX,CY,planets[i].orbit_r,0x00111118);
    }

    /* Draw planets */
    for(int i=6;i>=0;i--){
        int a=planets[i].angle;
        int ox=CX+planets[i].orbit_r*icos(a)/1000;
        int oy=CY+planets[i].orbit_r*isin(a)/2000; /* 2:1 perspective */
        draw_planet(ox,oy,&planets[i],i==travel_target);
    }

    /* Sun (on top of everything) */
    draw_sun(CX,CY,32,sun_speaking);

    /* AIMERANCIA label under sun */
    fb_drawstring(CX-36,CY+52,"AIMERANCIA",YGOLD,BLACK);
    fb_drawstring(CX-28,CY+63,"LISTENING...",GREEN,BLACK);

    /* HUD corners */
    unsigned int hc=LGOLD;
    int s=12;
    hl(0,s,0,hc); for(int i=0;i<=s;i++)px(0,i,hc);
    hl(W-1-s,W-1,0,hc); for(int i=0;i<=s;i++)px(W-1,i,hc);
    hl(0,s,H-1,hc); for(int i=H-1-s;i<=H-1;i++)px(0,i,hc);
    hl(W-1-s,W-1,H-1,hc); for(int i=H-1-s;i<=H-1;i++)px(W-1,i,hc);

    /* Top bar */
    fb_rectfill(0,0,W,14,0x00080808);
    hl(0,W-1,14,GOLD);
    fb_drawstring(8,3,"AIOS SOLAR SYSTEM",LGOLD,0x00080808);
    /* Clock */
    extern unsigned int timer_ticks_bss;
    int t=(int)timer_ticks_bss/100;
    char tb[10];
    tb[0]='0'+(t/3600)%24/10; tb[1]='0'+(t/3600)%24%10; tb[2]=':';
    tb[3]='0'+(t/60)%60/10;   tb[4]='0'+(t/60)%60%10;   tb[5]=':';
    tb[6]='0'+t%60/10;        tb[7]='0'+t%60%10;         tb[8]=0;
    fb_drawstring(W-80,3,tb,LGOLD,0x00080808);

    /* Planet guide bottom */
    fb_rectfill(0,H-14,W,14,0x00080808);
    hl(0,W-1,H-14,GOLD);
    fb_drawstring(8,H-11,"Say: go to [docs|learn|net|eng|auto|code|sched]",
                  DGRAY,0x00080808);
}

/* ── Travel animation ── */
static void draw_travel(void){
    int pid=travel_target;
    if(pid<0||pid>=7) return;
    planet_t*p=&planets[pid];
    int prog=travel_progress; /* 0-100 */

    /* Interpolate from sun to planet */
    int pa=p->angle;
    int tx=CX+p->orbit_r*icos(pa)/1000;
    int ty=CY+p->orbit_r*isin(pa)/2000;

    /* Camera moves toward planet */
    int vx=CX+(tx-CX)*prog/100;
    int vy=CY+(ty-CY)*prog/100;

    fb_rectfill(0,0,W,H,BLACK);

    /* Stars with motion blur */
    for(int i=0;i<NSTARS;i++){
        /* Stretch stars during travel */
        int sx=star_x[i], sy=star_y[i];
        if(prog>20){
            int dx=(sx-W/2)*prog/200;
            int dy=(sy-H/2)*prog/400;
            px(sx+dx,sy+dy,DIMSTAR);
            px(sx+dx/2,sy+dy/2,STARFLD);
        } else {
            px(sx,sy,DIMSTAR);
        }
    }

    /* Planet grows as we approach */
    int grow_size=p->size + p->size*prog/25;
    fc(W/2,H/2,grow_size+10,0x00111111);
    fc(W/2,H/2,grow_size,p->col1);
    fc(W/2-grow_size/4,H/2-grow_size/4,grow_size*3/4,p->col2);
    fc(W/2-grow_size/3,H/2-grow_size/3,grow_size/2,p->col3);

    /* Sun shrinks behind us */
    int shrink=32-32*prog/120; if(shrink<6)shrink=6;
    fc(CX-vx+CX/2,CY-vy+CY/2,shrink+8,SUN_ATM);
    draw_sun(CX-vx+CX/2, CY-vy+CY/2, shrink, 0);

    /* Progress */
    int barw=W/3;
    fb_rectfill(W/2-barw/2,H-30,barw,8,DGRAY);
    fb_rectfill(W/2-barw/2,H-30,barw*prog/100,8,YGOLD);
    fb_drawstring(W/2-30,H-42,"TRAVELING...",LGOLD,BLACK);

    /* Planet name */
    fb_drawstring(W/2-dlen(p->name)*4,H/2+grow_size+10,
                  p->name,YGOLD,BLACK);

    (void)vx;(void)vy;
}

/* ── Public API ── */
extern char "LISTENING..."[32];

void space_ui_init(void){
    W=fb_width; H=fb_height;
    CX=W/2; CY=H*45/100; /* slightly above center for perspective */
    init_stars();
    sun_pulse=0; sun_pdir=1; tick=0;
    travel_target=PLANET_NONE;
    travel_progress=0;
    current_planet=PLANET_NONE;
    returning=0;
    space_mode=1;
}

void space_ui_draw(void){
    if(!space_mode)return;
    if(travel_target!=PLANET_NONE && travel_progress<100){
        draw_travel();
    } else if(current_planet!=PLANET_NONE){
        draw_planet_surface(current_planet);
    } else {
        draw_solar_system();
    }
}

void space_ui_tick(void){
    if(!space_mode)return;
    tick++;

    /* Sun pulse */
    sun_pulse+=sun_pdir;
    if(sun_pulse>=10)sun_pdir=-1;
    if(sun_pulse<=0) sun_pdir=1;

    /* Orbit planets */
    for(int i=0;i<7;i++){
        if(tick%2==0){
            planets[i].angle=(planets[i].angle+planets[i].speed/5)%360;
        }
    }

    /* Travel animation */
    if(travel_target!=PLANET_NONE && travel_progress<100){
        travel_progress+=3;
        if(travel_progress>=100){
            travel_progress=100;
            current_planet=travel_target;
            /* If going to documents, hand off to doc_page */
            if(current_planet==PLANET_DOCUMENTS){
                doc_page_open_browser();
            }
        }
    }

    /* Redraw every tick */
    if(current_planet==PLANET_NONE||travel_progress<100){
        if(tick%3==0) space_ui_draw();
    }
}

void space_ui_travel_to(int pid){
    if(pid<0||pid>=7)return;
    travel_target=pid;
    travel_progress=0;
    current_planet=PLANET_NONE;
}

void space_ui_return(void){
    current_planet=PLANET_NONE;
    travel_target=PLANET_NONE;
    travel_progress=0;
    returning=0;
    space_ui_draw();
}

void space_ui_set_speaking(int on){
    sun_speaking=on;
}

int space_ui_active(void){
    return space_mode;
}

static int smatch(const char*s,const char*p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int shas(const char*s,const char*p){
    while(*s){if(smatch(s,p))return 1;s++;}return 0;
}

int space_ui_handle(const char*input){
    if(!space_mode)return 0;

    /* Return to solar system */
    if(smatch(input,"return")||smatch(input,"go back")||
       smatch(input,"solar system")||smatch(input,"back to space")||
       smatch(input,"retour")){
        space_ui_return();
        return 1;
    }

    /* Travel commands */
    if(shas(input,"document")||shas(input,"doc planet")||
       shas(input,"go to doc")||shas(input,"documents planet")){
        space_ui_travel_to(PLANET_DOCUMENTS); return 1;
    }
    if(shas(input,"learn")||shas(input,"knowledge planet")){
        space_ui_travel_to(PLANET_LEARNING); return 1;
    }
    if(shas(input,"network")||shas(input,"net planet")||shas(input,"reseau")){
        space_ui_travel_to(PLANET_NETWORK); return 1;
    }
    if(shas(input,"engineer")||shas(input,"eng planet")){
        space_ui_travel_to(PLANET_ENGINEERING); return 1;
    }
    if(shas(input,"auto")||shas(input,"autonomy")||shas(input,"guardian")){
        space_ui_travel_to(PLANET_AUTONOMY); return 1;
    }
    if(shas(input,"code")||shas(input,"compiler")||shas(input,"codegen")){
        space_ui_travel_to(PLANET_CODE); return 1;
    }
    if(shas(input,"sched")||shas(input,"scheduler")||shas(input,"tasks")){
        space_ui_travel_to(PLANET_SCHEDULER); return 1;
    }

    return 0;
}
