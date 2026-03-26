
#include "space_ui.h"
#include "framebuffer.h"
#include "../terminal/terminal.h"
#include "../ai/documents/doc_page.h"
#include "../mm/pmm.h"

int space_mode = 0;
extern int aios_ui_active;

/* ══════════════════════════════════════════════
   TRIG  (scaled x1024)
   ══════════════════════════════════════════════ */
static short SIN[360], COS2[360];
static void init_trig(void){
    static const short s36[37]={
        0,178,342,500,643,766,866,940,985,1024,
        985,940,866,766,643,500,342,178,0,-178,
        -342,-500,-643,-766,-866,-940,-985,-1024,
        -985,-940,-866,-766,-643,-500,-342,-178,0
    };
    for(int i=0;i<360;i++){
        int a=i/10, f=i%10;
        SIN[i]=(short)(s36[a]+(s36[a+1]-s36[a])*f/10);
        int ca=(i+90)%360, cb=ca/10, cf=ca%10;
        COS2[i]=(short)(s36[cb]+(s36[cb+1]-s36[cb])*cf/10);
    }
}
#define ISIN(a) SIN[((a)%360+360)%360]
#define ICOS(a) COS2[((a)%360+360)%360]

/* ══════════════════════════════════════════════
   CAMERA & PROJECTION
   ══════════════════════════════════════════════ */
static int cam_ay = 0;   /* auto-rotate Y degrees */
static int cam_ax = 28;  /* tilt X degrees - view from above */
#define CAM_FOV  520
#define CAM_ZDEP 420

static unsigned int W, H;

typedef struct { int x,y,depth; int ok; } P2;
static P2 proj(int wx, int wy, int wz){
    P2 r; r.ok=0;
    /* rotate Y */
    int cy=ICOS(cam_ay), sy=ISIN(cam_ay);
    int rx=(wx*cy/1024)-(wz*sy/1024);
    int rz=(wx*sy/1024)+(wz*cy/1024);
    /* rotate X */
    int cx2=ICOS(cam_ax), sx2=ISIN(cam_ax);
    int ry=(wy*cx2/1024)-(rz*sx2/1024);
    int rz2=(wy*sx2/1024)+(rz*cx2/1024);
    int dz=CAM_FOV+rz2+CAM_ZDEP;
    if(dz<10)return r;
    r.x=(int)(W/2)+ rx*CAM_FOV/dz;
    r.y=(int)(H/2)+ ry*CAM_FOV/dz;
    r.depth=rz2;
    r.ok=1;
    return r;
}

/* ══════════════════════════════════════════════
   COLOURS
   ══════════════════════════════════════════════ */
#define BG       0x00000508
#define BLACK    0x00000000
#define WHITE    0x00FFFFFF
#define YGOLD    0x00FFD700
#define LGOLD    0x00D4A017
#define GOLD     0x00B87333
#define GREEN    0x0000CC44
#define DGRAY    0x00111111
#define MGRAY    0x00333333

/* ══════════════════════════════════════════════
   PRIMITIVES
   ══════════════════════════════════════════════ */
static void dot(int x,int y,unsigned int c){
    if(x<0||y<0||(unsigned)x>=W||(unsigned)y>=H)return;
    fb_putpixel(x,y,c);
}
static void hline(int x1,int x2,int y,unsigned int c){
    if(x1>x2){int t=x1;x1=x2;x2=t;}
    for(int x=x1;x<=x2;x++)dot(x,y,c);
}
static void circle_fill(int cx,int cy,int r,unsigned int c){
    for(int dy=-r;dy<=r;dy++)
        for(int dx=-r;dx<=r;dx++)
            if(dx*dx+dy*dy<=r*r) dot(cx+dx,cy+dy,c);
}
static void ring_draw(int cx,int cy,int r,int t,unsigned int c){
    int r2=(r+t)*(r+t);
    for(int dy=-(r+t);dy<=(r+t);dy++)
        for(int dx=-(r+t);dx<=(r+t);dx++){
            int d=dx*dx+dy*dy;
            if(d>=r*r&&d<=r2) dot(cx+dx,cy+dy,c);
        }
}
static unsigned int blend_col(unsigned int a,unsigned int b,int t){
    int ar=(a>>16)&0xFF,ag=(a>>8)&0xFF,ab=a&0xFF;
    int br=(b>>16)&0xFF,bg=(b>>8)&0xFF,bb=b&0xFF;
    return (unsigned int)(((ar+(br-ar)*t/256)<<16)|
                          ((ag+(bg-ag)*t/256)<<8)|
                           (ab+(bb-ab)*t/256));
}
static int dlen(const char*s){int n=0;while(*s++)n++;return n;}

/* ══════════════════════════════════════════════
   3D SPHERE  (per-pixel phong shading)
   ══════════════════════════════════════════════ */
static void sphere3d(int cx,int cy,int r,
                     unsigned int cL,unsigned int cM,unsigned int cD){
    if(r<2)return;
    for(int dy=-r;dy<=r;dy++){
        int dx_max2=r*r-dy*dy;
        int dx_max=0; {int v=dx_max2,s=1;while(s*s<v)s++;dx_max=s;}
        for(int dx=-dx_max;dx<=dx_max;dx++){
            /* approx dz via integer sqrt */
            int d2=r*r-dx*dx-dy*dy; if(d2<0)d2=0;
            int dz=0;{int v=d2,s=1;while(s*s<v)s++;dz=s;}
            /* light from top-left */
            int dot2=(-dx-dy+dz);
            int mx=r+r+r;
            int lit=(dot2+mx)*256/(2*mx);
            if(lit<0)lit=0; if(lit>255)lit=255;
            unsigned int col;
            if(lit>200)      col=blend_col(cL,WHITE,(lit-200)*5);
            else if(lit>90)  col=blend_col(cM,cL,(lit-90)*256/110);
            else             col=blend_col(cD,cM,lit*256/90);
            dot(cx+dx,cy+dy,col);
        }
    }
}

/* ══════════════════════════════════════════════
   PARTICLE RING  (the blue orb effect)
   Particles orbit the sun in a wide halo
   ══════════════════════════════════════════════ */
#define NPART 220
static struct { int angle; int orbit; int size; unsigned int col; } parts[NPART];
static int tick=0;

static void init_particles(void){
    unsigned int seed=0xBEEF1234;
    for(int i=0;i<NPART;i++){
        seed=seed*1664525u+1013904223u;
        parts[i].angle  = (int)(seed%360);
        seed=seed*1664525u+1013904223u;
        /* orbit radius: 44-80 pixels around sun */
        parts[i].orbit  = 44+(int)(seed%36);
        seed=seed*1664525u+1013904223u;
        parts[i].size   = 1+(int)(seed%2);
        /* colour: mix of blue/cyan/white like image 2 */
        int t=(int)(seed%3);
        if(t==0)      parts[i].col=0x000088FF; /* blue */
        else if(t==1) parts[i].col=0x0022DDFF; /* cyan */
        else          parts[i].col=0x00AAEEFF; /* light cyan */
    }
}

static void draw_particle_ring(int cx, int cy, int speaking){
    /* Particles rotate around the sun orb */
    for(int i=0;i<NPART;i++){
        int a=(parts[i].angle + tick + i/3)%360;
        /* 3D projection of particle */
        int wx=parts[i].orbit*ICOS(a)/1024;
        int wz=parts[i].orbit*ISIN(a)/1024;
        /* slight vertical spread */
        int wy=(parts[i].orbit/6)*ISIN((a*2+i*17)%360)/1024;
        P2 p=proj(wx,wy,wz);
        if(!p.ok)continue;
        unsigned int c=parts[i].col;
        /* brighten when speaking */
        if(speaking) c=blend_col(c,WHITE,60);
        /* fade particles behind sun */
        if(p.depth>0) c=blend_col(c,BG,p.depth*3);
        int s=parts[i].size;
        for(int dy=-s;dy<=s;dy++)
            for(int dx=-s;dx<=s;dx++)
                dot(p.x+dx,p.y+dy,c);
    }
}

/* ══════════════════════════════════════════════
   SUN ORB  (glowing blue core + particle ring)
   ══════════════════════════════════════════════ */
static int sun_pulse=0, sun_pdir=1, sun_speaking=0;

static void draw_sun_orb(int cx, int cy){
    int p=sun_pulse;

    /* Outer dark atmosphere */
    for(int i=5;i>=1;i--){
        int gr=36+i*7+p;
        unsigned int gc=blend_col(0x00001133,BG,255-i*30);
        circle_fill(cx,cy,gr,gc);
    }

    /* Particle ring BEHIND sun */
    draw_particle_ring(cx,cy,sun_speaking);

    /* Corona glow ring */
    for(int i=3;i>=1;i--){
        int gr=30+p+i*3;
        unsigned int gc=blend_col(0x000044AA,BG,255-i*60);
        circle_fill(cx,cy,gr,gc);
    }

    /* Corona rays - blue tinted */
    for(int i=0;i<16;i++){
        int a=(i*22+tick*2)%360;
        int r1=28, r2=36+p+(sun_speaking?p:0);
        for(int s=r1;s<r2;s++){
            int fx=cx+s*ICOS(a)/1024;
            int fy=cy+s*ISIN(a)/1024;
            unsigned int cc=blend_col(0x000066CC,0x000022AA,(r2-s)*256/(r2-r1));
            dot(fx,fy,cc);
        }
    }

    /* Main orb - deep blue sphere */
    sphere3d(cx,cy,26+p/3, 0x0055BBFF, 0x00116699, 0x00001133);

    /* Inner glow */
    circle_fill(cx,cy,14,0x0033AADD);
    circle_fill(cx,cy,8, 0x0088DDFF);
    circle_fill(cx,cy,4, WHITE);

    /* Speaking rings pulse outward */
    if(sun_speaking){
        ring_draw(cx,cy,32+p,2,0x0022BBFF);
        ring_draw(cx,cy,40+p,1,0x00116688);
    }

    /* Labels */
    fb_drawstring(cx-36,cy+42,"AIMERANCIA",YGOLD,BG);
    fb_drawstring(cx-28,cy+52,
                  sun_speaking?"SPEAKING...":"LISTENING...",
                  GREEN,BG);
}

/* ══════════════════════════════════════════════
   STARS
   ══════════════════════════════════════════════ */
#define NSTARS 200
static short stx[NSTARS],sty[NSTARS],stz[NSTARS];
static unsigned char stb[NSTARS];
static void init_stars(void){
    unsigned int seed=0xC0DE1234;
    for(int i=0;i<NSTARS;i++){
        seed=seed*1664525u+1013904223u;
        int a=(int)(seed%360);
        seed=seed*1664525u+1013904223u;
        int b=(int)(seed%180)-90;
        int r=600+(int)(seed%200);
        stx[i]=(short)(r*ICOS(a)/1024*ICOS(b+90)/1024);
        sty[i]=(short)(r*ISIN(b+90)/1024);
        stz[i]=(short)(r*ISIN(a)/1024*ICOS(b+90)/1024);
        stb[i]=(unsigned char)(seed%4);
    }
}

static void draw_stars(int warp){
    for(int i=0;i<NSTARS;i++){
        P2 p=proj(stx[i],sty[i],stz[i]);
        if(!p.ok)continue;
        if(p.x<0||p.x>=(int)W||p.y<0||p.y>=(int)H)continue;
        unsigned int c=(stb[i]==0)?0x00222222:
                       (stb[i]==1)?0x00555555:
                       (stb[i]==2)?0x00888888:0x00AAAAAA;
        if((tick+i*7)%60<2) c=WHITE;
        if(warp>20){
            int dx=(p.x-(int)W/2)*warp/500;
            int dy=(p.y-(int)H/2)*warp/1000;
            hline(p.x,p.x+dx,p.y,c);
        } else {
            dot(p.x,p.y,c);
        }
    }
}

/* ══════════════════════════════════════════════
   ORBIT RINGS
   ══════════════════════════════════════════════ */
static void draw_orbit(int orb, int tilt){
    for(int a=0;a<360;a+=2){
        int wx=orb*ICOS(a)/1024;
        int wz=orb*ISIN(a)/1024;
        int wy=orb*ISIN(a)/1024*ISIN(tilt)/1024;
        P2 p=proj(wx,wy,wz);
        if(p.ok) dot(p.x,p.y,0x00112233);
        a++; /* dashed */
    }
}

/* ══════════════════════════════════════════════
   PLANETS
   ══════════════════════════════════════════════ */
typedef struct {
    const char*name; const char*label;
    int orbit; int size;
    unsigned int cL,cM,cD;
    int angle; int speed; int tilt;
} Planet;

static Planet PL[7]={
    {"Documents","DOCS",  170,12,0x00FFCC66,0x00CC8833,0x00664411,  0, 7,12},
    {"Learning", "LEARN", 225,9, 0x0066CC88,0x00338866,0x00115544, 52, 5,-10},
    {"Network",  "NET",   285,11,0x0033AAEE,0x001166BB,0x00003388,124, 4,  7},
    {"Engineering","ENG", 230,10,0x00EE8833,0x00BB5511,0x00662200,208, 6, 18},
    {"Autonomy", "AUTO",  170,8, 0x00CC44AA,0x00882277,0x00440033,288, 8,-14},
    {"Code",     "CODE",  285,9, 0x0022CCAA,0x00119977,0x00005544,312, 3,  4},
    {"Scheduler","SCHED", 255,8, 0x00AAAA22,0x00777700,0x00333300, 36, 2, 10},
};

/* ══════════════════════════════════════════════
   DEPTH SORT  (8 objects: sun + 7 planets)
   ══════════════════════════════════════════════ */
typedef struct{int id;int depth;}DE;
static DE dbuf[8];
static void dsort(void){
    for(int i=1;i<8;i++){
        DE k=dbuf[i]; int j=i-1;
        while(j>=0&&dbuf[j].depth>k.depth){dbuf[j+1]=dbuf[j];j--;}
        dbuf[j+1]=k;
    }
}

/* ══════════════════════════════════════════════
   STATE
   ══════════════════════════════════════════════ */
static int travel_target=PLANET_NONE;
static int travel_progress=0;
static int current_planet=PLANET_NONE;
static char space_status[32]="LISTENING...";

/* ══════════════════════════════════════════════
   MAIN SOLAR SYSTEM VIEW
   ══════════════════════════════════════════════ */
static void draw_solar(void){
    fb_rectfill(0,0,W,H,BG);
    draw_stars(0);

    /* orbit rings */
    for(int i=0;i<7;i++) draw_orbit(PL[i].orbit,PL[i].tilt);

    /* build depth buffer */
    P2 sp=proj(0,0,0);
    dbuf[0].id=-1; dbuf[0].depth=sp.ok?sp.depth:0;
    for(int i=0;i<7;i++){
        int a=PL[i].angle;
        int wx=PL[i].orbit*ICOS(a)/1024;
        int wz=PL[i].orbit*ISIN(a)/1024;
        int wy=PL[i].orbit*ISIN(a)/1024*ISIN(PL[i].tilt)/1024;
        P2 pp=proj(wx,wy,wz);
        dbuf[i+1].id=i;
        dbuf[i+1].depth=pp.ok?pp.depth:0;
    }
    dsort();

    /* draw far→near */
    for(int d=0;d<8;d++){
        int id=dbuf[d].id;
        if(id==-1){
            /* SUN ORB */
            if(sp.ok) draw_sun_orb(sp.x,sp.y);
        } else {
            Planet*pl=&PL[id];
            int a=pl->angle;
            int wx=pl->orbit*ICOS(a)/1024;
            int wz=pl->orbit*ISIN(a)/1024;
            int wy=pl->orbit*ISIN(a)/1024*ISIN(pl->tilt)/1024;
            P2 pp=proj(wx,wy,wz);
            if(!pp.ok)continue;
            /* perspective scale */
            int pr=pl->size*CAM_FOV/(CAM_FOV+CAM_ZDEP+pp.depth);
            if(pr<2)pr=2;
            /* ground shadow */
            P2 sh=proj(wx,-50,wz);
            if(sh.ok){
                for(int dy=-1;dy<=1;dy++)
                    hline(sh.x-pr,sh.x+pr,sh.y+dy,0x00080808);
            }
            sphere3d(pp.x,pp.y,pr,pl->cL,pl->cM,pl->cD);
            if(id==travel_target){
                ring_draw(pp.x,pp.y,pr+3,2,YGOLD);
                ring_draw(pp.x,pp.y,pr+7,1,GOLD);
            }
            /* label - offset so it never overlaps */
            int lx=pp.x-dlen(pl->label)*4;
            int ly=pp.y+pr+10;
            if(ly<(int)H-18)
                fb_drawstring(lx,ly,pl->label,
                    id==travel_target?YGOLD:LGOLD, BG);
        }
    }

    /* HUD frame corners */
    unsigned int hc=LGOLD; int s=14;
    hline(0,s,0,hc); for(int i=0;i<=s;i++)dot(0,i,hc);
    hline(W-1-s,W-1,0,hc); for(int i=0;i<=s;i++)dot(W-1,i,hc);
    hline(0,s,H-1,hc); for(int i=H-1-s;i<=(int)H-1;i++)dot(0,i,hc);
    hline(W-1-s,W-1,H-1,hc); for(int i=H-1-s;i<=(int)H-1;i++)dot(W-1,i,hc);

    /* top bar */
    fb_rectfill(0,0,W,16,0x00080808);
    hline(0,W-1,16,GOLD);
    fb_drawstring(10,4,"AIOS SOLAR SYSTEM",LGOLD,0x00080808);
    extern unsigned int timer_ticks_bss;
    int t=(int)timer_ticks_bss/100; char tb[10];
    tb[0]='0'+(t/3600)%24/10; tb[1]='0'+(t/3600)%24%10; tb[2]=':';
    tb[3]='0'+(t/60)%60/10;   tb[4]='0'+(t/60)%60%10;   tb[5]=':';
    tb[6]='0'+t%60/10;        tb[7]='0'+t%60%10;         tb[8]=0;
    fb_drawstring(W-80,4,tb,LGOLD,0x00080808);
    /* bottom bar */
    fb_rectfill(0,H-16,W,16,0x00080808);
    hline(0,W-1,H-16,GOLD);
    fb_drawstring(10,H-12,
        "Say: go to [docs|learn|net|eng|auto|code|sched]",
        MGRAY,0x00080808);
}

/* ══════════════════════════════════════════════
   TRAVEL ANIMATION
   ══════════════════════════════════════════════ */
static void draw_travel(void){
    int pid=travel_target; if(pid<0||pid>=7)return;
    Planet*pl=&PL[pid];
    int prog=travel_progress;
    fb_rectfill(0,0,W,H,BG);
    draw_stars(prog);
    /* planet looms larger */
    int gr=pl->size*2 + pl->size*prog/10;
    sphere3d(W/2,H/2,gr,pl->cL,pl->cM,pl->cD);
    fb_drawstring(W/2-dlen(pl->name)*4, H/2+gr+14,
                  pl->name, YGOLD, BG);
    /* mini sun top-left with particle ring (scaled down) */
    draw_sun_orb(46, 60);
    /* progress bar */
    int bw=W/3;
    fb_rectfill(W/2-bw/2,H-36,bw,8,DGRAY);
    fb_rectfill(W/2-bw/2,H-36,bw*prog/100,8,0x000088FF);
    fb_drawstring(W/2-34,H-48,"TRAVELING...",LGOLD,BG);
}

/* ══════════════════════════════════════════════
   PLANET SURFACE
   ══════════════════════════════════════════════ */
static void draw_surface(void){
    int pid=current_planet; if(pid<0||pid>=7)return;
    Planet*pl=&PL[pid];
    fb_rectfill(0,0,W,H,BG);
    draw_stars(0);
    /* horizon */
    for(int i=0;i<20;i++){
        unsigned int hc2=blend_col(pl->cD,pl->cM,i*12);
        hline(0,W-1,H-40+i,hc2);
    }
    /* big planet sphere */
    sphere3d(W/2,H/3,90,pl->cL,pl->cM,pl->cD);
    /* mini sun orb corner */
    draw_sun_orb(44,58);
    /* title */
    fb_drawstring(W/2-dlen(pl->name)*5, H*55/100,
                  pl->name, YGOLD, BG);
    hline(W/2-70,W/2+70, H*55/100+14, GOLD);
    fb_drawstring(W/2-44, H*55/100+24,
                  "[ MODULE LOADED ]", MGRAY, BG);
}

/* ══════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════ */
void space_ui_init(void){
    W=fb_width; H=fb_height;
    init_trig();
    init_stars();
    init_particles();
    sun_pulse=0; sun_pdir=1; tick=0;
    cam_ay=0; cam_ax=28;
    travel_target=PLANET_NONE;
    travel_progress=0;
    current_planet=PLANET_NONE;
    space_mode=1;
    aios_ui_active=0;
}

void space_ui_draw(void){
    if(!space_mode)return;
    if(travel_target!=PLANET_NONE&&travel_progress<100) draw_travel();
    else if(current_planet!=PLANET_NONE) draw_surface();
    else draw_solar();
}

void space_ui_tick(void){
    if(!space_mode)return;
    tick++;
    /* sun pulse */
    sun_pulse+=sun_pdir;
    if(sun_pulse>=10)sun_pdir=-1;
    if(sun_pulse<=0) sun_pdir=1;
    /* camera auto-rotate */
    cam_ay=(cam_ay+1)%360;
    /* orbit planets */
    for(int i=0;i<7;i++)
        if(tick%2==0)
            PL[i].angle=(PL[i].angle+PL[i].speed)%360;
    /* travel */
    if(travel_target!=PLANET_NONE&&travel_progress<100){
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
    if(pid<0||pid>=7)return;
    travel_target=pid; travel_progress=0; current_planet=PLANET_NONE;
}
void space_ui_return(void){
    current_planet=PLANET_NONE; travel_target=PLANET_NONE;
    travel_progress=0; space_ui_draw();
}
void space_ui_set_speaking(int on){ sun_speaking=on; }
int  space_ui_active(void){ return space_mode; }

static int smatch(const char*a,const char*b){while(*b){if(*a!=*b)return 0;a++;b++;}return 1;}
static int shas(const char*s,const char*p){while(*s){if(smatch(s,p))return 1;s++;}return 0;}

int space_ui_handle(const char*in){
    if(!space_mode)return 0;
    if(smatch(in,"return")||smatch(in,"go back")||
       smatch(in,"solar system")||smatch(in,"back to space")||
       smatch(in,"retour")){space_ui_return();return 1;}
    if(shas(in,"document")||shas(in,"go to doc"))
        {space_ui_travel_to(PLANET_DOCUMENTS);return 1;}
    if(shas(in,"learn")||shas(in,"knowledge"))
        {space_ui_travel_to(PLANET_LEARNING);return 1;}
    if(shas(in,"network")||shas(in,"reseau"))
        {space_ui_travel_to(PLANET_NETWORK);return 1;}
    if(shas(in,"engineer"))
        {space_ui_travel_to(PLANET_ENGINEERING);return 1;}
    if(shas(in,"auto")||shas(in,"autonomy")||shas(in,"guardian"))
        {space_ui_travel_to(PLANET_AUTONOMY);return 1;}
    if(shas(in,"code")||shas(in,"compiler"))
        {space_ui_travel_to(PLANET_CODE);return 1;}
    if(shas(in,"sched")||shas(in,"scheduler"))
        {space_ui_travel_to(PLANET_SCHEDULER);return 1;}
    return 0;
}
