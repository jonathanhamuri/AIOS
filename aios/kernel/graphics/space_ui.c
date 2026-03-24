
#include "space_ui.h"
#include "framebuffer.h"
#include "../terminal/terminal.h"
#include "../ai/documents/doc_page.h"
#include "../mm/pmm.h"

int space_mode = 0;
extern int aios_ui_active;

/* ══════════════════════════════════════════════════════
   FIXED-POINT 3D ENGINE
   All coords * 1024 (10-bit fraction)
   ══════════════════════════════════════════════════════ */
#define FP   1024
#define FP2  (FP*FP)

/* Trig table 0-359 deg, scaled x1024 */
static short sin_t[360];
static short cos_t[360];
static void init_trig(void){
    /* Pre-baked 32-entry then interpolated would be better
       but for boot simplicity we use a 36-entry table + lerp */
    static const short s36[36]={
        0,178,342,512,656,777,882,954,1000,1022,
        1022,1000,954,882,777,656,512,342,178,0,
        -178,-342,-512,-656,-777,-882,-954,-1000,-1022,-1022,
        -1000,-954,-882,-777,-656,-512
    };
    for(int i=0;i<360;i++){
        int idx=i/10, frac=i%10;
        int a=s36[idx], b=s36[(idx+1)%36];
        sin_t[i]=(short)(a+(b-a)*frac/10);
        /* cos(i) = sin(i+90) */
        cos_t[i]=(short)(s36[((idx+9)%36)] + (s36[((idx+10)%36)]-s36[((idx+9)%36)])*frac/10);
    }
}
#define ISIN(a) sin_t[((a)%360+360)%360]
#define ICOS(a) cos_t[((a)%360+360)%360]

/* ── Camera ── */
static int cam_angX = 25;   /* degrees, tilt down */
static int cam_angY = 0;    /* degrees, auto-rotate */
static int cam_fov  = 480;  /* perspective strength */
static int cam_dist = 0;    /* extra z push */

/* 3D → 2D projection (coords in world units, not FP) */
typedef struct { int x, y, depth; int valid; } Proj;
static unsigned int W, H;

static Proj project3d(int wx, int wy, int wz){
    Proj p; p.valid=0;
    /* Rotate Y */
    int cy=ICOS(cam_angY), sy=ISIN(cam_angY);
    int rx = (wx*cy - wz*sy)/FP;
    int rz = (wx*sy + wz*cy)/FP;
    /* Rotate X */
    int cx2=ICOS(cam_angX), sx2=ISIN(cam_angX);
    int ry2= (wy*cx2 - rz*sx2)/FP;
    int rz2= (wy*sx2 + rz*cx2)/FP;
    int dz = cam_fov + rz2 + 380;
    if(dz < 10) return p;
    p.x = (int)W/2 + rx * cam_fov / dz;
    p.y = (int)H/2 + ry2 * cam_fov / dz;
    p.depth = rz2;
    p.valid = 1;
    return p;
}

/* ── Colors (0x00RRGGBB) ── */
#define BLACK    0x00000000
#define WHITE    0x00FFFFFF
#define YGOLD    0x00FFD700
#define LGOLD    0x00D4A017
#define GOLD     0x00B87333
#define GREEN    0x0000CC44
#define DGRAY    0x00111111
#define BGDARK   0x00000508

/* ── Primitives ── */
static void px2(int x,int y,unsigned int c){
    if(x<0||y<0||(unsigned)x>=W||(unsigned)y>=H)return;
    fb_putpixel(x,y,c);
}
static void hl(int x1,int x2,int y,unsigned int c){
    if(x1>x2){int t=x1;x1=x2;x2=t;}
    for(int x=x1;x<=x2;x++) px2(x,y,c);
}
/* Filled circle */
static void fc(int cx,int cy,int r,unsigned int c){
    for(int dy=-r;dy<=r;dy++)
        for(int dx=-r;dx<=r;dx++)
            if(dx*dx+dy*dy<=r*r) px2(cx+dx,cy+dy,c);
}
/* Ring */
static void ring(int cx,int cy,int r,int t,unsigned int c){
    int r2=(r+t)*(r+t);
    for(int dy=-(r+t);dy<=(r+t);dy++)
        for(int dx=-(r+t);dx<=(r+t);dx++){
            int d=dx*dx+dy*dy;
            if(d>=r*r&&d<=r2) px2(cx+dx,cy+dy,c);
        }
}

/* ── Blend two colours ── */
static unsigned int blend(unsigned int a, unsigned int b, int t){
    /* t: 0=all a, 256=all b */
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab2=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb2=b&0xFF;
    int r=ar+(br-ar)*t/256;
    int g=ag+(bg-ag)*t/256;
    int bv=ab2+(bb2-ab2)*t/256;
    return (unsigned int)((r<<16)|(g<<8)|bv);
}

/* ── 3D Sphere with shading ── */
static void draw_sphere3d(int cx, int cy, int r,
                           unsigned int cLight, unsigned int cMid, unsigned int cDark){
    if(r<2) return;
    int r2=r*r;
    /* Light direction: top-left */
    for(int dy=-r;dy<=r;dy++){
        for(int dx=-r;dx<=r;dx++){
            int d2=dx*dx+dy*dy;
            if(d2>r2) continue;
            /* Normal dot light (simplified) */
            int dz2 = r2 - d2; /* dz^2 */
            /* Approximate dz */
            int dz=0;
            {int v=dz2,s=1;while(s*s<v)s++;dz=s;}
            /* Light = top-left = (-1,-1,1) normalized */
            /* dot = (-dx - dy + dz) / r / sqrt3  */
            int dot = (-dx - dy + dz);
            /* Map to 0-256 */
            int maxdot = r + r + r;
            int lit = (dot + maxdot)*256/(2*maxdot);
            if(lit<0)lit=0; if(lit>255)lit=255;
            unsigned int col;
            if(lit>180)      col=blend(cLight,WHITE, (lit-180)*4);
            else if(lit>80)  col=blend(cMid,  cLight,(lit-80)*256/100);
            else             col=blend(cDark, cMid,  lit*256/80);
            px2(cx+dx,cy+dy,col);
        }
    }
}

/* ── Stars ── */
#define NSTARS 180
static short sx3[NSTARS], sy3[NSTARS], sz3[NSTARS];
static unsigned char sbright[NSTARS];
static void init_stars(void){
    unsigned int seed=0xC0FFEE;
    for(int i=0;i<NSTARS;i++){
        seed=seed*1664525u+1013904223u;
        int a=(int)(seed%360); int b=(int)((seed>>8)%360);
        int r=750+(int)((seed>>16)%150);
        sx3[i]=(short)(r*ICOS(a)/FP*ICOS(b)/FP);
        sy3[i]=(short)(r*ISIN(b)/FP);
        sz3[i]=(short)(r*ISIN(a)/FP*ICOS(b)/FP);
        sbright[i]=(unsigned char)((seed>>24)%4);
    }
}

/* ── Sun pulse ── */
static int sun_pulse=0, sun_pdir=1, sun_speaking=0;

/* ── Draw 3D sun ── */
static void draw_sun3d(int cx, int cy, int r){
    int p=sun_pulse/2;
    /* Atmosphere layers */
    for(int i=4;i>=1;i--){
        int gr=r+8+i*6+p;
        int alpha=i*3; /* fake alpha by darkening */
        unsigned int gc=blend(0x00441100,BLACK,220-alpha*10);
        fc(cx,cy,gr,gc);
    }
    /* Corona rays */
    for(int i=0;i<16;i++){
        int a=(i*22+sun_pulse*3)%360;
        int r1=r+3, r2=r+10+p+(sun_speaking?p:0);
        int dx=ICOS(a), dz=ISIN(a);
        for(int s=r1;s<r2;s++){
            int xi=cx+s*dx/FP, yi=cy+s*dz/FP;
            unsigned int cc=blend(0x00FF6600,0x00FF2200,(r2-s)*256/(r2-r1));
            px2(xi,yi,cc);
        }
    }
    /* Sun sphere layers */
    draw_sphere3d(cx,cy,r,0x00FFFFAA,0x00FFD700,0x00FF6600);
    /* Bright core */
    fc(cx,cy,r/3,WHITE);
    /* Speaking rings */
    if(sun_speaking){
        ring(cx,cy,r+14+p,2,YGOLD);
        ring(cx,cy,r+20+p,1,LGOLD);
    }
}

/* ── Orbit ring 3D ── */
static void draw_orbit3d(int orb, int tilt_deg){
    unsigned int c=0x00111122;
    for(int a=0;a<360;a+=3){
        int wx=orb*ICOS(a)/FP;
        int wz=orb*ISIN(a)/FP;
        int wy=orb*ISIN(a)/FP*ISIN(tilt_deg)/FP;
        Proj p=project3d(wx,wy,wz);
        if(p.valid) px2(p.x,p.y,c);
        /* Dashed: skip every other */
        a++;
    }
}

/* ── Planet definitions ── */
typedef struct {
    const char* name; const char* label;
    int orbit; int size;
    unsigned int cLight, cMid, cDark;
    int angle; int speed; /* speed in 0.1 deg/tick */
    int tilt;  /* orbit tilt degrees */
} planet_t;

static planet_t planets[7]={
    {"Documents","DOCS",  140,12,0x00FFCC66,0x00CC8833,0x00664411,  0,7, 12},
    {"Learning", "LEARN", 190,9, 0x0066CC88,0x003388AA,0x00115588, 60,5,-10},
    {"Network",  "NET",   245,10,0x0033AAEE,0x001166BB,0x00003388,120,4,  7},
    {"Engineering","ENG", 190,10,0x00EE8833,0x00BB5511,0x00662200,210,6, 18},
    {"Autonomy", "AUTO",  140,8, 0x00CC44AA,0x00882277,0x00440033,290,8,-14},
    {"Code",     "CODE",  245,9, 0x0022CCAA,0x00119977,0x00005544,300,3,  4},
    {"Scheduler","SCHED", 215,8, 0x00AAAA22,0x00777700,0x00333300, 30,2, 10},
};

/* ── Travel/view state ── */
static int tick=0;
static int travel_target=PLANET_NONE;
static int travel_progress=0;
static int current_planet=PLANET_NONE;
static char space_status[32]="LISTENING...";

/* String length */
static int dlen(const char*s){int n=0;while(*s++)n++;return n;}

/* ── Draw stars ── */
static void draw_stars(int warp){
    for(int i=0;i<NSTARS;i++){
        unsigned int c=(sbright[i]==0)?0x00222222:
                       (sbright[i]==1)?0x00555555:
                       (sbright[i]==2)?0x00888888:0x00AAAAAA;
        if((tick+i*7)%60<2) c=WHITE;
        Proj p=project3d(sx3[i],sy3[i],sz3[i]);
        if(!p.valid) continue;
        if(warp && warp>20){
            /* Stretch stars */
            int dx=(p.x-(int)W/2)*warp/600;
            int dy=(p.y-(int)H/2)*warp/1200;
            hl(p.x,p.x+dx,p.y,c);
            if(dy) for(int j=0;j<dy;j++) px2(p.x,p.y+j,c);
        } else {
            px2(p.x,p.y,c);
        }
    }
}

/* ── Depth-sort helper ── */
typedef struct { int idx; int depth; } DepthEntry;
static DepthEntry depth_buf[8]; /* 7 planets + sun */
static void sort_depth(void){
    /* Insertion sort (tiny array) */
    for(int i=1;i<8;i++){
        DepthEntry key=depth_buf[i]; int j=i-1;
        while(j>=0 && depth_buf[j].depth > key.depth){
            depth_buf[j+1]=depth_buf[j]; j--;
        }
        depth_buf[j+1]=key;
    }
}

/* ── Main solar system draw ── */
static void draw_solar_system(void){
    fb_rectfill(0,0,W,H,BGDARK);
    draw_stars(0);

    /* Orbit rings */
    for(int i=0;i<7;i++) draw_orbit3d(planets[i].orbit,planets[i].tilt);

    /* Compute depths for sun + planets */
    Proj sp=project3d(0,0,0);
    depth_buf[0].idx=-1; depth_buf[0].depth=sp.valid?sp.depth:0;
    for(int i=0;i<7;i++){
        int a=planets[i].angle;
        int wx=planets[i].orbit*ICOS(a)/FP;
        int wz=planets[i].orbit*ISIN(a)/FP;
        int wy=planets[i].orbit*ISIN(a)/FP*ISIN(planets[i].tilt)/FP;
        Proj pp=project3d(wx,wy,wz);
        depth_buf[i+1].idx=i;
        depth_buf[i+1].depth=pp.valid?pp.depth:0;
    }
    sort_depth();

    /* Draw far → near */
    for(int d=0;d<8;d++){
        int idx=depth_buf[d].idx;
        if(idx==-1){
            /* Sun */
            if(sp.valid){
                int sr=32; /* base radius */
                draw_sun3d(sp.x,sp.y,sr);
                fb_drawstring(sp.x-36,sp.y+sr+18,"AIMERANCIA",YGOLD,BGDARK);
                fb_drawstring(sp.x-28,sp.y+sr+28,space_status,GREEN,BGDARK);
            }
        } else {
            planet_t*pl=&planets[idx];
            int a=pl->angle;
            int wx=pl->orbit*ICOS(a)/FP;
            int wz=pl->orbit*ISIN(a)/FP;
            int wy=pl->orbit*ISIN(a)/FP*ISIN(pl->tilt)/FP;
            Proj pp=project3d(wx,wy,wz);
            if(!pp.valid) continue;
            /* Scale radius by perspective */
            int pr=pl->size * cam_fov / (cam_fov+380+pp.depth);
            if(pr<2) pr=2;
            /* Drop shadow (ellipse under planet) */
            {
                Proj shp=project3d(wx,-40,wz);
                if(shp.valid)
                    for(int dy=-2;dy<=2;dy++)
                        hl(shp.x-pr,shp.x+pr,shp.y+dy,0x00080808);
            }
            draw_sphere3d(pp.x,pp.y,pr,pl->cLight,pl->cMid,pl->cDark);
            if(idx==travel_target){
                ring(pp.x,pp.y,pr+3,2,YGOLD);
                ring(pp.x,pp.y,pr+7,1,GOLD);
            }
            /* Label */
            int lx=pp.x-dlen(pl->label)*4;
            int ly=pp.y+pr+8;
            if(ly<(int)H-10)
                fb_drawstring(lx,ly,pl->label,
                              idx==travel_target?YGOLD:LGOLD,BGDARK);
        }
    }

    /* HUD corners */
    unsigned int hc=LGOLD; int s=12;
    hl(0,s,0,hc); for(int i=0;i<=s;i++)px2(0,i,hc);
    hl(W-1-s,W-1,0,hc); for(int i=0;i<=s;i++)px2(W-1,i,hc);
    hl(0,s,H-1,hc); for(int i=H-1-s;i<=(int)H-1;i++)px2(0,i,hc);
    hl(W-1-s,W-1,H-1,hc); for(int i=H-1-s;i<=(int)H-1;i++)px2(W-1,i,hc);

    /* Top bar */
    fb_rectfill(0,0,W,14,0x00080808); hl(0,W-1,14,GOLD);
    fb_drawstring(8,3,"AIOS SOLAR SYSTEM  [3D]",LGOLD,0x00080808);
    extern unsigned int timer_ticks_bss;
    int t=(int)timer_ticks_bss/100; char tb[10];
    tb[0]='0'+(t/3600)%24/10; tb[1]='0'+(t/3600)%24%10; tb[2]=':';
    tb[3]='0'+(t/60)%60/10;   tb[4]='0'+(t/60)%60%10;   tb[5]=':';
    tb[6]='0'+t%60/10;        tb[7]='0'+t%60%10;         tb[8]=0;
    fb_drawstring(W-80,3,tb,LGOLD,0x00080808);
    /* Bottom */
    fb_rectfill(0,H-14,W,14,0x00080808); hl(0,W-1,H-14,GOLD);
    fb_drawstring(8,H-11,"Say: go to [docs|learn|net|eng|auto|code|sched]",
                  0x00333333,0x00080808);
}

/* ── Travel animation ── */
static void draw_travel(void){
    int pid=travel_target; if(pid<0||pid>=7)return;
    planet_t*pl=&planets[pid];
    int prog=travel_progress;

    fb_rectfill(0,0,W,H,BGDARK);
    draw_stars(prog);

    /* Planet grows toward camera */
    int gr=pl->size*2 + pl->size*prog/8;
    draw_sphere3d(W/2,H/2,gr,pl->cLight,pl->cMid,pl->cDark);

    /* Planet name */
    fb_drawstring(W/2-dlen(pl->name)*4, H/2+gr+12, pl->name, YGOLD, BGDARK);

    /* Shrinking sun top-left */
    int shrink=28-20*prog/100; if(shrink<6)shrink=6;
    draw_sun3d(44, 54, shrink);
    fb_drawstring(4, 54+shrink+14, "AIMERANCIA", LGOLD, BGDARK);

    /* Progress bar */
    int barw=W/3;
    fb_rectfill(W/2-barw/2,H-32,barw,8,0x00111111);
    fb_rectfill(W/2-barw/2,H-32,barw*prog/100,8,YGOLD);
    fb_drawstring(W/2-30,H-44,"TRAVELING...",LGOLD,BGDARK);
}

/* ── Planet surface ── */
static void draw_surface(void){
    int pid=current_planet; if(pid<0||pid>=7)return;
    planet_t*pl=&planets[pid];

    fb_rectfill(0,0,W,H,BGDARK);
    /* Horizon: gradient bands */
    for(int i=0;i<24;i++){
        int yy=H-50+i;
        unsigned int hcol=blend(pl->cDark,pl->cMid,i*10);
        hl(0,W-1,yy,hcol);
    }
    /* Surface grid */
    for(int i=0;i<6;i++){
        unsigned int gc=blend(pl->cMid,BGDARK,200);
        hl(0,W-1,H-80+i*10,gc);
    }
    /* Large planet sphere center-top */
    draw_sphere3d(W/2,H*24/100,100,pl->cLight,pl->cMid,pl->cDark);

    /* Stars */
    draw_stars(0);

    /* Mini sun corner */
    draw_sun3d(40,52,14);
    fb_drawstring(4,52+22,"AIMERANCIA",LGOLD,BGDARK);
    fb_drawstring(4,52+32,space_status,GREEN,BGDARK);

    /* Planet title */
    fb_drawstring(W/2-dlen(pl->name)*5, H/2+10, pl->name, YGOLD, BGDARK);
    hl(W/2-60,W/2+60,H/2+22,GOLD);
    fb_drawstring(W/2-50,H/2+32,"[ MODULE LOADED ]",0x00333333,BGDARK);
}

/* ══════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════ */

void space_ui_init(void){
    W=fb_width; H=fb_height;
    init_trig();
    init_stars();
    sun_pulse=0; sun_pdir=1; tick=0;
    cam_angX=22; cam_angY=0;
    travel_target=PLANET_NONE;
    travel_progress=0;
    current_planet=PLANET_NONE;
    space_mode=1;
    aios_ui_active=0; /* space_ui owns the screen */
}

void space_ui_draw(void){
    if(!space_mode) return;
    if(travel_target!=PLANET_NONE && travel_progress<100) draw_travel();
    else if(current_planet!=PLANET_NONE) draw_surface();
    else draw_solar_system();
}

void space_ui_tick(void){
    if(!space_mode) return;
    tick++;

    /* Sun pulse */
    sun_pulse+=sun_pdir;
    if(sun_pulse>=12) sun_pdir=-1;
    if(sun_pulse<=0)  sun_pdir=1;

    /* Auto-rotate camera */
    cam_angY=(cam_angY+1)%360;

    /* Orbit planets */
    for(int i=0;i<7;i++)
        if(tick%2==0)
            planets[i].angle=(planets[i].angle+planets[i].speed)%360;

    /* Travel */
    if(travel_target!=PLANET_NONE && travel_progress<100){
        travel_progress+=2;
        if(travel_progress>=100){
            travel_progress=100;
            current_planet=travel_target;
            travel_target=PLANET_NONE;
            if(current_planet==PLANET_DOCUMENTS) doc_page_open_browser();
        }
    }

    if(tick%2==0) space_ui_draw();
}

void space_ui_travel_to(int pid){
    if(pid<0||pid>=7) return;
    travel_target=pid; travel_progress=0; current_planet=PLANET_NONE;
}
void space_ui_return(void){
    current_planet=PLANET_NONE; travel_target=PLANET_NONE; travel_progress=0;
    space_ui_draw();
}
void space_ui_set_speaking(int on){ sun_speaking=on; }
int  space_ui_active(void){ return space_mode; }

static int smatch(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static int shas(const char*s,const char*p){while(*s){if(smatch(s,p))return 1;s++;}return 0;}

int space_ui_handle(const char*input){
    if(!space_mode) return 0;
    if(smatch(input,"return")||smatch(input,"go back")||
       smatch(input,"solar system")||smatch(input,"back to space")||
       smatch(input,"retour")){space_ui_return();return 1;}
    if(shas(input,"document")||shas(input,"go to doc"))
        {space_ui_travel_to(PLANET_DOCUMENTS);return 1;}
    if(shas(input,"learn")||shas(input,"knowledge"))
        {space_ui_travel_to(PLANET_LEARNING);return 1;}
    if(shas(input,"network")||shas(input,"reseau"))
        {space_ui_travel_to(PLANET_NETWORK);return 1;}
    if(shas(input,"engineer"))
        {space_ui_travel_to(PLANET_ENGINEERING);return 1;}
    if(shas(input,"auto")||shas(input,"autonomy")||shas(input,"guardian"))
        {space_ui_travel_to(PLANET_AUTONOMY);return 1;}
    if(shas(input,"code")||shas(input,"compiler"))
        {space_ui_travel_to(PLANET_CODE);return 1;}
    if(shas(input,"sched")||shas(input,"scheduler"))
        {space_ui_travel_to(PLANET_SCHEDULER);return 1;}
    return 0;
}
