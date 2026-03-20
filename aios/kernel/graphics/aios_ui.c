#include "aios_ui.h"
#include "../graphics/framebuffer.h"
#include "../mm/pmm.h"
#include "../ai/learning/learning.h"
#include "../apps/apps.h"

int aios_ui_active = 0;

/* ── Colors (32bpp ARGB) ── */
#define BG      0x00000000
#define DARK    0x00111111
#define PANEL   0x00111008
#define GOLD    0x00B87333
#define LGOLD   0x00D4A017
#define YGOLD   0x00FFD700
#define GREEN   0x0000CC44
#define WHITE   0x00FFFFFF
#define GRAY    0x00444444
#define DGRAY   0x00222222
#define RED     0x00CC2222
#define DKTEXT  0x00333322

/* Layout — computed at runtime from fb_width/fb_height */
static unsigned int W, H;
static unsigned int LP_W;   /* left panel width  */
static unsigned int RP_X;   /* right panel start */
static unsigned int RP_W;   /* right panel width */
static unsigned int CT_X;   /* center start      */
static unsigned int CT_W;   /* center width      */
static unsigned int SB_H;   /* status bar height */
static unsigned int IB_Y;   /* input bar top     */
static unsigned int IB_H;   /* input bar height  */
static unsigned int OX, OY, ORB; /* orb           */
static unsigned int FONT;   /* font char width   */
static unsigned int LH;     /* line height       */

static int pulse=0, pdir=1, pangle=0, tick=0;
static char ibuf[128]; static int ilen=0;
static char status_str[32]="LISTENING...";

#define OL 8
#define OC 52
static char obuf[OL][OC+1];
static unsigned int ocol[OL];
static int oline=0;

static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}

/* ── Drawing primitives ── */
static void hl(int x1,int x2,int y,unsigned int c){
    if(y<0||(unsigned int)y>=H)return;
    for(int x=x1;x<=x2;x++) fb_putpixel(x,y,c);
}
static void vl(int x,int y1,int y2,unsigned int c){
    if(x<0||(unsigned int)x>=W)return;
    for(int y=y1;y<=y2;y++) fb_putpixel(x,y,c);
}
static void fc(int cx,int cy,int r,unsigned int c){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r) fb_putpixel(cx+x,cy+y,c);
}
static void ring(int cx,int cy,int r,int t,unsigned int c){
    int r2=(r+t)*(r+t);
    for(int y=-(r+t);y<=(r+t);y++)
        for(int x=-(r+t);x<=(r+t);x++){
            int d=x*x+y*y;
            if(d>=r*r&&d<=r2) fb_putpixel(cx+x,cy+y,c);
        }
}
static void dring(int cx,int cy,int r,unsigned int c){
    /* dashed ring — draw dots every 8 degrees */
    for(int a=0;a<360;a+=5){
        int aa=a%90,q=a/90,ca,sa;
        if(aa<45){ca=90-aa;sa=aa*2;}else{ca=aa-45;sa=90-(aa-45)*2;}
        int px,py;
        if(q==0){px=ca;py=sa;}else if(q==1){px=-sa;py=ca;}
        else if(q==2){px=-ca;py=-sa;}else{px=sa;py=-ca;}
        if(a%10<5) fb_putpixel(cx+px*r/90,cy+py*r/90,c);
    }
}
static void brk(int x,int y,int w,int h,unsigned int c){
    int s=(int)LP_W/10; if(s<8)s=8; if(s>18)s=18;
    hl(x,x+s,y,c);     vl(x,y,y+s,c);
    hl(x+w-s,x+w,y,c); vl(x+w,y,y+s,c);
    hl(x,x+s,y+h,c);   vl(x,y+h-s,y+h,c);
    hl(x+w-s,x+w,y+h,c); vl(x+w,y+h-s,y+h,c);
}
static void bar(int x,int y,int w,int h,int pct,unsigned int bg,unsigned int fg){
    fb_rectfill(x,y,w,h,bg);
    fb_rectfill(x,y,pct*w/100,h,fg);
}

/* ── STATUS BAR ── */
static void draw_statusbar(void){
    fb_rectfill(0,0,W,SB_H,PANEL);
    hl(0,W-1,SB_H,GOLD);
    /* Gold dot */
    fc(SB_H/2,SB_H/2,5,LGOLD);
    /* Name */
    fb_drawstring(SB_H+6,SB_H/2-4,"AIMERANCIA",YGOLD,PANEL);
    /* Green dot + ONLINE */
    fc(SB_H*3,SB_H/2,5,GREEN);
    fb_drawstring(SB_H*3+10,SB_H/2-4,"ONLINE",GREEN,PANEL);
    /* Clock */
    extern unsigned int timer_ticks_bss;
    int t=(int)timer_ticks_bss/100;
    char tb[12];
    tb[0]='0'+(t/3600)%24/10; tb[1]='0'+(t/3600)%24%10; tb[2]=':';
    tb[3]='0'+(t/60)%60/10;   tb[4]='0'+(t/60)%60%10;   tb[5]=':';
    tb[6]='0'+t%60/10;        tb[7]='0'+t%60%10;         tb[8]=0;
    fb_drawstring(W-120,SB_H/2-4,tb,LGOLD,PANEL);
    fb_drawstring(W-55, SB_H/2-4,"v1.0",GRAY,PANEL);
}

/* ── LEFT PANEL ── */
static void draw_left_panel(void){
    fb_rectfill(0,SB_H+1,LP_W,IB_Y-SB_H-2,DARK);
    vl(LP_W,SB_H+1,IB_Y-1,GOLD);
    brk(2,SB_H+4,LP_W-4,IB_Y-SB_H-8,LGOLD);

    int cx=LP_W/2;
    /* SYSTEM title */
    fb_drawstring(cx-24,SB_H+10,"SYSTEM",LGOLD,DARK);
    hl(6,LP_W-6,SB_H+22,GOLD);

    int fp=pmm_free_pages();
    int mp=(256-fp)*100/256;
    int y=SB_H+28;
    int bw=LP_W-44; /* bar width */
    int bh=6;

    /* MEM */
    fb_drawstring(6,y,"MEM",GOLD,DARK);
    bar(42,y+1,bw,bh,mp,DGRAY,mp>80?RED:GOLD);
    char pb[6]; pb[0]='0'+mp/10; pb[1]='0'+mp%10; pb[2]='%'; pb[3]=0;
    fb_drawstring(LP_W-30,y,pb,LGOLD,DARK);
    y+=LH;

    /* CPU */
    fb_drawstring(6,y,"CPU",GOLD,DARK);
    bar(42,y+1,bw,bh,2,DGRAY,GREEN);
    fb_drawstring(LP_W-30,y," 0%",LGOLD,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;

    /* NET */
    fb_drawstring(6,y,"NET",GOLD,DARK);
    fc(42,y+5,5,GREEN);
    fb_drawstring(52,y,"LIVE",GREEN,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;

    /* SKILLS */
    fb_drawstring(6,y,"SKILLS",GOLD,DARK);
    fb_drawint(LP_W-30,y,learning_count,LGOLD,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;

    /* TASKS */
    fb_drawstring(6,y,"TASKS",GOLD,DARK);
    fb_drawint(LP_W-30,y,task_count,GREEN,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;

    /* UPTIME */
    extern unsigned int timer_ticks_bss;
    int t=(int)timer_ticks_bss/100;
    fb_drawstring(6,y,"UPTIME",GOLD,DARK);
    fb_drawint(LP_W-46,y,t/60,LGOLD,DARK);
    fb_drawstring(LP_W-30,y,"m",LGOLD,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;

    /* HEALTH */
    fb_drawstring(6,y,"HEALTH",GOLD,DARK);
    bar(6,y+LH-4,LP_W-12,bh,100,DGRAY,GREEN);
    fb_drawstring(LP_W-38,y,"100%",LGOLD,DARK);
    y+=LH+8; hl(6,LP_W-6,y,GRAY); y+=8;

    /* Recent activity */
    fb_drawstring(cx-40,y,"RECENT ACTIVITY",GRAY,DARK);
    y+=LH; hl(6,LP_W-6,y,GRAY); y+=6;
    fb_drawstring(6,y,"> boot complete",DKTEXT,DARK);    y+=LH-2;
    fb_drawstring(6,y,"> net discovered",DKTEXT,DARK);   y+=LH-2;
    fb_drawstring(6,y,"> kb loaded",DKTEXT,DARK);        y+=LH-2;
    fb_drawstring(6,y,"> scheduler ready",DKTEXT,DARK);

    brk(2,IB_Y-10,LP_W-4,8,LGOLD);
}

/* ── RIGHT PANEL ── */
static void draw_right_panel(void){
    fb_rectfill(RP_X,SB_H+1,RP_W,IB_Y-SB_H-2,DARK);
    vl(RP_X,SB_H+1,IB_Y-1,GOLD);
    brk(RP_X+2,SB_H+4,RP_W-4,IB_Y-SB_H-8,LGOLD);

    int cx=RP_X+RP_W/2;
    fb_drawstring(cx-28,SB_H+10,"MODULES",LGOLD,DARK);
    hl(RP_X+6,W-6,SB_H+22,GOLD);

    const char* mn[8]={"INTENT ENGINE","KNOWLEDGE BASE","LEARNING SYS",
                        "SCHEDULER","NET DISCOVERY","VOICE [PENDING]",
                        "VISUAL DET.","AUTONOMY II"};
    unsigned int mc[8]={GREEN,GREEN,GREEN,GREEN,GREEN,GOLD,GOLD,GRAY};
    int y=SB_H+30;
    for(int i=0;i<8;i++){
        fc(RP_X+14,y+6,5,mc[i]);
        fb_drawstring(RP_X+24,y,mn[i],mc[i],DARK);
        y+=LH+2;
    }

    hl(RP_X+6,W-6,y+2,GRAY); y+=10;
    fb_drawstring(cx-36,y,"TASK QUEUE",GRAY,DARK); y+=LH;
    hl(RP_X+6,W-6,y,GRAY); y+=6;
    fb_drawstring(RP_X+8,y,"o optimize kb index",GOLD,DARK); y+=LH-2;
    fb_drawstring(RP_X+8,y,"o scan net ports",GOLD,DARK);    y+=LH-2;
    fb_drawstring(RP_X+8,y,"o generate report",GOLD,DARK);

    brk(RP_X+2,IB_Y-10,RP_W-4,8,LGOLD);
}

/* ── CENTER ── */
static void draw_log(void){
    /* Clear only log area — stable, no flicker */
    unsigned int log_h=(IB_Y-SB_H)*35/100;
    fb_rectfill(CT_X,SB_H+1,CT_W,log_h,BG);
    vl(CT_X,SB_H+1,SB_H+(int)log_h,GRAY);
    vl(CT_X+CT_W-1,SB_H+1,SB_H+(int)log_h,GRAY);
    hl(CT_X,CT_X+CT_W-1,SB_H+log_h,GRAY);
    for(int i=0;i<OL;i++){
        int idx=(oline+i)%OL;
        if(obuf[idx][0])
            fb_drawstring(CT_X+8,SB_H+6+i*(LH-1),obuf[idx],ocol[idx],BG);
    }
}
static void draw_orb_only(void){
    /* Clear only orb area */
    unsigned int log_h=(IB_Y-SB_H)*35/100;
    fb_rectfill(CT_X,SB_H+(int)log_h+1,CT_W,IB_Y-SB_H-(int)log_h-2,BG);
    vl(CT_X,SB_H+(int)log_h+1,IB_Y-1,GRAY);
    vl(CT_X+CT_W-1,SB_H+(int)log_h+1,IB_Y-1,GRAY);

    /* Outer dashed rings */
    dring(OX,OY,ORB+40,GRAY);
    dring(OX,OY,ORB+28,GOLD);
    /* Solid rings */
    ring(OX,OY,ORB+14,2,GOLD);
    ring(OX,OY,ORB+6, 2,GOLD);
    /* Rotating particles */
    for(int i=0;i<6;i++){
        int a=(pangle+i*60)%360;
        int r=ORB+22;
        int aa=a%90,q=a/90,ca,sa;
        if(aa<45){ca=90-aa;sa=aa*2;}else{ca=aa-45;sa=90-(aa-45)*2;}
        int px,py;
        if(q==0){px=ca;py=sa;}else if(q==1){px=-sa;py=ca;}
        else if(q==2){px=-ca;py=-sa;}else{px=sa;py=-ca;}
        fc(OX+px*r/90,OY+py*r/90,4,(i%2)?YGOLD:LGOLD);
    }
    /* Core layers — filled properly */
    fc(OX,OY,ORB,      DGRAY);
    fc(OX,OY,ORB-6,    0x00331100);
    fc(OX,OY,ORB-14,   GOLD);
    fc(OX,OY,ORB-22,   LGOLD);
    fc(OX,OY,ORB-30,   YGOLD);
    if(pulse>5) fc(OX,OY,ORB-36,WHITE);
    /* Crosshair */
    hl(OX-ORB+8,OX+ORB-8,OY,LGOLD);
    vl(OX,OY-ORB+8,OY+ORB-8,LGOLD);
    /* HUD brackets */
    brk(OX-ORB-20,OY-ORB-20,2*(ORB+20),2*(ORB+20),LGOLD);
    /* Name + status */
    fb_drawstring(OX-40,OY+ORB+16,"AIMERANCIA",YGOLD,BG);
    fb_drawstring(OX-48,OY+ORB+32,status_str,GREEN,BG);
}
static void draw_center(void){
    draw_log();
    draw_orb_only();
}

/* ── INPUT BAR ── */
static void draw_inputbar(void){
    fb_rectfill(0,IB_Y,W,IB_H,PANEL);
    hl(0,W-1,IB_Y,GOLD);
    fb_drawstring(10,IB_Y+IB_H/2-4,"aimerancia",GREEN,PANEL);
    fb_drawstring(90,IB_Y+IB_H/2-4,"@sys :~$",LGOLD,PANEL);
    int cx=148;
    int start=ilen>40?ilen-40:0;
    for(int i=start;i<ilen;i++){
        char ch[2]={ibuf[i],0};
        fb_drawstring(cx,IB_Y+IB_H/2-4,ch,WHITE,PANEL);
        cx+=8;
    }
    if(tick%14<7) fb_rectfill(cx,IB_Y+4,6,IB_H-8,YGOLD);
    else           fb_rectfill(cx,IB_Y+4,6,IB_H-8,PANEL);
}

/* ── INIT (compute layout from actual fb size) ── */
void aios_ui_init(void){
    aios_ui_active=1;
    W=fb_width; H=fb_height;
    /* Layout proportions matching mockup */
    LP_W = W*26/100;   /* 26% left  */
    RP_W = W*22/100;   /* 22% right */
    RP_X = W-RP_W;
    CT_X = LP_W+1;
    CT_W = RP_X-LP_W-2;
    unsigned int CT_H = IB_Y-SB_H-2;
    SB_H = H*5/100;  if(SB_H<24)SB_H=24;
    IB_H = H*6/100;  if(IB_H<20)IB_H=20;
    IB_Y = H-IB_H;
    LH   = H*2/100;  if(LH<14)LH=14; if(LH>20)LH=20;
    FONT = 8;
    /* Orb in lower-center */
    OX   = CT_X+CT_W/2;
    OY   = SB_H + (IB_Y-SB_H)*70/100;
    ORB  = (CT_W<CT_H?CT_W:CT_H)*18/100;
    if(ORB<30)ORB=30; if(ORB>80)ORB=80;

    pulse=0;pdir=1;pangle=0;tick=0;
    ilen=0;ibuf[0]=0;oline=0;
    for(int i=0;i<OL;i++){obuf[i][0]=0;ocol[i]=WHITE;}

    fb_clear(BG);
    draw_statusbar();
    draw_left_panel();
    draw_right_panel();
    draw_center();
    draw_inputbar();

    aios_ui_print("> system: all modules loaded",8);
    aios_ui_print("> aimerancia: ready to assist",14);
}

void aios_ui_draw(void){
    fb_clear(BG);
    draw_statusbar();draw_left_panel();draw_right_panel();draw_center();draw_inputbar();
}

void aios_ui_tick(void){
    if(!aios_ui_active)return;
    tick++;pulse+=pdir;
    if(pulse>=10)pdir=-1;if(pulse<=0)pdir=1;
    if(tick%8==0)pangle=(pangle+6)%360;
    /* Draw orb+inputbar every 6 ticks — stable, not too fast */
    if(tick%6==0){ draw_orb_only(); draw_inputbar(); }
    if(tick%12==0) draw_statusbar();
    if(tick%240==0){draw_left_panel();draw_right_panel();}
}

void aios_ui_set_status(const char*s){scopy(status_str,s,32);}

void aios_ui_print(const char*t,unsigned char color_unused){
    unsigned int c;
    /* pick color by prefix */
    if(t[0]=='>'&&t[1]==' '&&t[2]=='a') c=LGOLD;
    else if(t[0]=='>') c=GRAY;
    else c=WHITE;
    int i=0;while(t[i]&&i<OC){obuf[oline][i]=t[i];i++;}
    obuf[oline][i]=0;ocol[oline]=c;
    oline=(oline+1)%OL;obuf[oline][0]=0;
    draw_log();
}

void aios_ui_prompt(void){aios_ui_set_status("LISTENING...");ilen=0;ibuf[0]=0;}
void aios_ui_input_char(char c){if(ilen<127){ibuf[ilen++]=c;ibuf[ilen]=0;}}
void aios_ui_input_backspace(void){if(ilen>0){ilen--;ibuf[ilen]=0;}}
void aios_ui_input_clear(void){
    if(ilen>0){
        char tmp[OC+1];tmp[0]='>';tmp[1]=' ';
        int i=0;while(ibuf[i]&&i<OC-2){tmp[i+2]=ibuf[i];i++;}tmp[i+2]=0;
        aios_ui_print(tmp,14);
    }
    ilen=0;ibuf[0]=0;aios_ui_set_status("PROCESSING...");
}
const char* aios_ui_get_input(void){return ibuf;}
