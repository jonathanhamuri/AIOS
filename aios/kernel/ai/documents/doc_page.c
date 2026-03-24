#include "doc_page.h"
#include "../../graphics/framebuffer.h"
#include "../../terminal/terminal.h"
#include "../../disk/kbfs.h"
#include "../../ai/knowledge/kb.h"
#include "../../mm/pmm.h"

int current_page = PAGE_MAIN;

/* ── Colors — dark AIOS theme matching screenshot ── */
#define BG        0x00000000   /* pure black */
#define PANEL_BG  0x00111108   /* dark panel */
#define PANEL_BD  0x00B87333   /* gold border */
#define DOC_BG    0x000A0A08   /* doc area bg */
#define DOC_BD    0x00333320   /* doc border */
#define TOOLBAR   0x00111108   /* toolbar bg */
#define TB_BORDER 0x00B87333   /* gold */
#define HEADER_C  0x00D4A017   /* gold text headers */
#define BODY_C    0x00CCCCAA   /* body text */
#define TITLE_C   0x00FFD700   /* bright gold title */
#define DIM_C     0x00666655   /* dim text */
#define SEL_BG    0x00222218   /* selected doc bg */
#define GREEN     0x0000CC44
#define GOLD      0x00B87333
#define LGOLD     0x00D4A017
#define YGOLD     0x00FFD700
#define WHITE     0x00FFFFFF
#define DIVIDER   0x00333322
#define STATUS_BG 0x00111108
#define SAVED_C   0x0000AA55   /* green "Saved" */
#define ICON_C    0x00888866

/* ── Document store ── */
doc_entry_t doc_store[DOC_STORE_MAX];
int doc_count   = 0;
int active_doc  = -1;
static int view_scroll = 0;
static int page_tick   = 0;

/* ── Layout ── */
static unsigned int W, H;
static unsigned int LP_W;    /* left panel width */
static unsigned int TB_H;    /* title/tab bar height */
static unsigned int FMT_H;   /* formatting toolbar height */
static unsigned int RULER_H; /* ruler height */
static unsigned int ST_H;    /* status bar height */
static unsigned int DOC_X, DOC_W, DOC_Y, DOC_H; /* doc content area */
static unsigned int MARGIN;  /* left text margin inside doc */

static void layout(void){
    W = fb_width; H = fb_height;
    LP_W   = W*28/100; if(LP_W<200)LP_W=200;
    TB_H   = H*7/100;  if(TB_H<28)TB_H=28;
    FMT_H  = H*5/100;  if(FMT_H<22)FMT_H=22;
    RULER_H= H*2/100;  if(RULER_H<8)RULER_H=8;
    ST_H   = H*5/100;  if(ST_H<22)ST_H=22;
    DOC_X  = LP_W;
    DOC_W  = W - LP_W;
    DOC_Y  = TB_H + FMT_H + RULER_H;
    DOC_H  = H - DOC_Y - ST_H;
    MARGIN = W*4/100; if(MARGIN<24)MARGIN=24;
}

/* ── Helpers ── */
static int dlen(const char*s){int n=0;while(*s++)n++;return n;}
static void dcopy(char*d,const char*s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}
static int dstart(const char*s,const char*p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int deq(const char*a,const char*b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static void dcat(char*d,const char*s,int m){
    int i=dlen(d);while(*s&&i<m-1){d[i++]=*s++;}d[i]=0;
}
static void hl(int x1,int x2,int y,unsigned int c){
    if(y<0||(unsigned)y>=H)return;
    for(int x=x1;x<=x2;x++)fb_putpixel(x,y,c);
}
static void vl(int x,int y1,int y2,unsigned int c){
    if(x<0||(unsigned)x>=W)return;
    for(int y=y1;y<=y2;y++)fb_putpixel(x,y,c);
}
static int word_count(const char*s){
    int n=0,in=0;
    while(*s){if(*s==' '||*s=='\n'){in=0;}else{if(!in){n++;in=1;}}s++;}
    return n;
}
static void int_to_str(int n,char*buf){
    if(n==0){buf[0]='0';buf[1]=0;return;}
    char tmp[12];int ti=0;
    while(n>0){tmp[ti++]='0'+n%10;n/=10;}
    int bi=0;
    for(int x=ti-1;x>=0;x--)buf[bi++]=tmp[x];
    buf[bi]=0;
}
static void draw_circle(int cx,int cy,int r,unsigned int c){
    for(int y=-r;y<=r;y++)
        for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r)fb_putpixel(cx+x,cy+y,c);
}

/* ── Left panel ── */
static void draw_left_panel(void){
    fb_rectfill(0,0,LP_W,H,PANEL_BG);
    vl(LP_W,0,H,PANEL_BD);

    /* AIOS top bar — mirror main UI */
    extern unsigned int timer_ticks_bss;
    int up=(int)timer_ticks_bss/100;
    fb_rectfill(0,0,LP_W,TB_H+FMT_H+RULER_H,PANEL_BG);

    /* SYSTEM label */
    int sy=12;
    fb_drawstring(LP_W/2-24,sy,"SYSTEM",LGOLD,PANEL_BG); sy+=16;
    hl(4,LP_W-4,sy,GOLD); sy+=6;

    /* Stats */
    extern int pmm_free_pages();
    int fp=pmm_free_pages();
    int mp=(256-fp)*100/256;
    fb_drawstring(6,sy,"MEM",GOLD,PANEL_BG);
    fb_rectfill(36,sy+1,LP_W-50,5,0x00222218);
    fb_rectfill(36,sy+1,mp*(LP_W-50)/100,5,mp>80?0x00CC2222:GOLD);
    char pb[8]; int_to_str(mp,pb);
    dcat(pb,"%",8);
    fb_drawstring(LP_W-28,sy,pb,LGOLD,PANEL_BG);
    sy+=12;

    fb_drawstring(6,sy,"CPU",GOLD,PANEL_BG);
    fb_rectfill(36,sy+1,LP_W-50,5,0x00222218);
    fb_rectfill(36,sy+1,2,5,GREEN);
    fb_drawstring(LP_W-28,sy," 0%",LGOLD,PANEL_BG);
    sy+=12;

    hl(4,LP_W-4,sy,DIVIDER); sy+=4;

    fb_drawstring(6,sy,"NET",GOLD,PANEL_BG);
    draw_circle(38,sy+4,4,GREEN);
    fb_drawstring(46,sy,"LIVE",GREEN,PANEL_BG);
    sy+=12; hl(4,LP_W-4,sy,DIVIDER); sy+=4;

    fb_drawstring(6,sy,"SKILLS",GOLD,PANEL_BG);
    char skb[8]; int_to_str(3,skb);
    fb_drawstring(LP_W-20,sy,skb,LGOLD,PANEL_BG);
    sy+=12; hl(4,LP_W-4,sy,DIVIDER); sy+=4;

    fb_drawstring(6,sy,"TASKS",GOLD,PANEL_BG);
    fb_drawstring(LP_W-16,sy,"0",LGOLD,PANEL_BG);
    sy+=12; hl(4,LP_W-4,sy,DIVIDER); sy+=4;

    fb_drawstring(6,sy,"UPTIME",GOLD,PANEL_BG);
    char ub[8]; int_to_str(up/60,ub); dcat(ub,"m",8);
    fb_drawstring(LP_W-32,sy,ub,LGOLD,PANEL_BG);
    sy+=12; hl(4,LP_W-4,sy,DIVIDER); sy+=4;

    fb_drawstring(6,sy,"HEALTH",GOLD,PANEL_BG);
    fb_rectfill(6,sy+9,LP_W-12,5,GREEN);
    fb_drawstring(LP_W-32,sy,"100%",LGOLD,PANEL_BG);
    sy+=20; hl(4,LP_W-4,sy,DIVIDER); sy+=6;

    /* DOCUMENTS section */
    fb_drawstring(LP_W/2-36,sy,"DOCUMENTS",LGOLD,PANEL_BG); sy+=12;
    hl(4,LP_W-4,sy,GOLD); sy+=6;

    for(int i=0;i<doc_count&&i<8;i++){
        unsigned int bg=(i==active_doc)?SEL_BG:PANEL_BG;
        fb_rectfill(4,sy,LP_W-8,14,bg);
        /* color dot by type */
        unsigned int dot=LGOLD;
        if(doc_store[i].type[0]=='R') dot=0x00CC6600;
        if(doc_store[i].type[0]=='L') dot=GREEN;
        if(doc_store[i].type[0]=='P') dot=0x00AA8800;
        draw_circle(12,sy+7,4,dot);
        /* truncate title to fit */
        char ttl[32]={0};
        int maxc=(LP_W-30)/8; if(maxc>28)maxc=28;
        for(int j=0;j<maxc&&doc_store[i].title[j];j++)ttl[j]=doc_store[i].title[j];
        fb_drawstring(22,sy+2,ttl,i==active_doc?YGOLD:BODY_C,bg);
        sy+=16;
    }

    /* CREATE NEW DOCUMENT button */
    int by=H-ST_H-28;
    fb_rectfill(4,by,LP_W-8,22,0x00222218);
    hl(4,LP_W-8,by,GOLD); hl(4,LP_W-8,by+22,GOLD);
    vl(4,by,by+22,GOLD); vl(LP_W-4,by,by+22,GOLD);
    fb_drawstring(12,by+6,"[ CREATE NEW DOCUMENT]",YGOLD,0x00222218);

    /* Recent activity */
    sy=by-42;
    fb_drawstring(6,sy,"> 2 created new document",DIM_C,PANEL_BG); sy+=12;
    fb_drawstring(6,sy,"> 1 AI database accessed",DIM_C,PANEL_BG);
}

/* ── Tab bar (top) ── */
static void draw_tabbar(const char*title){
    fb_rectfill(LP_W,0,DOC_W,TB_H,TOOLBAR);
    hl(LP_W,W-1,TB_H,TB_BORDER);

    /* Doc icon + DOCUMENTS tab */
    fb_rectfill(LP_W+6,TB_H/2-8,14,16,LGOLD);
    fb_drawstring(LP_W+6,TB_H/2-4,"D",TOOLBAR,LGOLD);
    fb_drawstring(LP_W+24,TB_H/2-4,"DOCUMENTS",LGOLD,TOOLBAR);

    /* Divider */
    vl(LP_W+110,TB_H/4,TB_H*3/4,DIVIDER);

    /* Saved indicator */
    extern unsigned int timer_ticks_bss;
    int up=(int)timer_ticks_bss/100;
    char sb[24]="Saved ";
    char tb[8]; int_to_str(up/3600%24,tb);
    dcat(sb,tb,24); dcat(sb,":",24);
    int_to_str(up/60%60,tb); dcat(sb,tb,24); dcat(sb,":",24);
    int_to_str(up%60,tb); dcat(sb,tb,24);
    fb_drawstring(LP_W+118,TB_H/2-4,sb,SAVED_C,TOOLBAR);

    /* Right icons (□ □ search ≡ ...) */
    fb_drawstring(W-100,TB_H/2-4,"[=] [-] [?] [:]",ICON_C,TOOLBAR);

    /* Document title below tab */
    fb_rectfill(LP_W,TB_H,DOC_W,FMT_H+RULER_H,0x000D0D0A);
    hl(LP_W,W-1,TB_H+FMT_H+RULER_H,TB_BORDER);
    /* Document icon + title */
    fb_rectfill(LP_W+8,TB_H+FMT_H/2-7,14,14,LGOLD);
    fb_drawstring(LP_W+8,TB_H+FMT_H/2-3,"E",0x000D0D0A,LGOLD);
    fb_drawstring(LP_W+26,TB_H+FMT_H/2-4,title,WHITE,0x000D0D0A);
}

/* ── Formatting toolbar ── */
static void draw_fmt_toolbar(void){
    int fy=TB_H+FMT_H+2;
    fb_rectfill(LP_W,TB_H+FMT_H,DOC_W,RULER_H,TOOLBAR);
    /* Format buttons matching screenshot */
    int x=LP_W+8;
    /* Page icon */
    fb_rectfill(x,fy-1,12,12,ICON_C); x+=18;
    /* Font box */
    fb_rectfill(x,fy-1,14,12,0x00222218);
    hl(x,x+14,fy-1,DIVIDER); hl(x,x+14,fy+11,DIVIDER);
    vl(x,fy-1,fy+11,DIVIDER); vl(x+14,fy-1,fy+11,DIVIDER);
    fb_drawstring(x+2,fy+1,"A",BODY_C,0x00222218); x+=18;
    /* Font name */
    fb_rectfill(x,fy-1,56,12,0x00222218);
    fb_drawstring(x+2,fy+1,"Arial",BODY_C,0x00222218); x+=60;
    /* Size */
    fb_rectfill(x,fy-1,20,12,0x00222218);
    fb_drawstring(x+2,fy+1,"12",BODY_C,0x00222218); x+=24;
    /* B I U S */
    fb_drawstring(x,fy+1,"B",WHITE,TOOLBAR); x+=14;
    fb_drawstring(x,fy+1,"I",BODY_C,TOOLBAR); x+=12;
    fb_drawstring(x,fy+1,"U",BODY_C,TOOLBAR); x+=12;
    fb_drawstring(x,fy+1,"S",BODY_C,TOOLBAR); x+=18;
    /* Alignment icons */
    for(int j=0;j<6;j++){
        hl(x,x+10,fy+2+(j%3)*3,ICON_C);
        if(j==2)x+=16; else x+=14;
    }
    /* List/indent */
    for(int j=0;j<4;j++){
        fb_drawstring(x,fy+1,"-",ICON_C,TOOLBAR); x+=12;
    }
}

/* ── Text rendering with sections ── */
static void draw_doc_content(const char*body,const char*title){
    /* Clear doc area */
    fb_rectfill(DOC_X,DOC_Y,DOC_W,DOC_H,DOC_BG);
    vl(DOC_X,DOC_Y,DOC_Y+DOC_H,TB_BORDER);

    int tx = DOC_X + MARGIN;
    int tw = DOC_W - MARGIN*2;
    int ty = DOC_Y + 20 - view_scroll*14;
    int charw=8, lh=16;
    int chpl = tw/charw;

    /* Draw title large */
    if(ty>=DOC_Y-20 && ty<(int)(DOC_Y+DOC_H)){
        int tlen=dlen(title)*charw;
        fb_drawstring(DOC_X+MARGIN, ty, title, YGOLD, DOC_BG);
        /* underline title */
        hl(DOC_X+MARGIN, DOC_X+MARGIN+tlen+20, ty+lh-2, TB_BORDER);
    }
    ty += lh+8;

    /* Parse and render body */
    int i=0;
    while(body[i] && ty < (int)(DOC_Y+DOC_H+20)){
        /* Section header: line starting with uppercase word followed by newline */
        /* Detect headers — lines that are short and end with \n, all caps or title */
        if(ty < DOC_Y-16){
            /* skip to next line */
            while(body[i] && body[i]!='\n') i++;
            if(body[i]=='\n') i++;
            ty += lh;
            continue;
        }

        /* Collect current line */
        char line[128]={0};
        int li=0;
        while(body[i] && body[i]!='\n' && li<127){
            line[li++]=body[i++];
        }
        line[li]=0;
        if(body[i]=='\n') i++;

        int linelen=dlen(line);

        if(linelen==0){
            ty += lh/2;
            continue;
        }

        /* Detect section header: short line, first char uppercase, no lowercase run */
        int is_header=0;
        if(linelen>2 && linelen<40 && line[0]>='A' && line[0]<='Z'){
            int upper=0,lower=0;
            for(int j=0;j<linelen;j++){
                if(line[j]>='A'&&line[j]<='Z') upper++;
                if(line[j]>='a'&&line[j]<='z') lower++;
            }
            /* Header if mostly uppercase words or title case short line */
            if(upper>lower || linelen<20) is_header=1;
        }

        if(is_header){
            /* Section header — gold, larger visual weight, divider under */
            if(ty>=DOC_Y){
                ty+=4;
                fb_drawstring(DOC_X+MARGIN, ty, line, HEADER_C, DOC_BG);
                hl(DOC_X+MARGIN, DOC_X+MARGIN+tw, ty+lh-1, DIVIDER);
                ty += lh+6;
            } else {
                ty += lh+10;
            }
        } else {
            /* Body text — word wrap */
            int cx=0;
            for(int j=0;j<linelen;j++){
                if(cx+1 > chpl){
                    /* wrap */
                    if(ty>=DOC_Y){
                        /* already rendered up to cx chars */
                    }
                    ty+=lh; cx=0;
                    if(ty>=(int)(DOC_Y+DOC_H)) break;
                }
                if(ty>=DOC_Y){
                    char ch[2]={line[j],0};
                    fb_drawstring(DOC_X+MARGIN+cx*charw, ty, ch, BODY_C, DOC_BG);
                }
                cx++;
            }
            ty+=lh;
        }
    }
}

/* ── Status bar ── */
static void draw_status_bar(int wc,const char*mode){
    int sy=H-ST_H;
    fb_rectfill(LP_W,sy,DOC_W,ST_H,STATUS_BG);
    hl(LP_W,W-1,sy,TB_BORDER);
    fb_drawstring(LP_W+10, sy+ST_H/2-4, mode, BODY_C, STATUS_BG);
    /* Word count right-aligned */
    char wb[24]="Words: ";
    char wn[8]; int_to_str(wc,wn);
    dcat(wb,wn,24);
    int wlen=dlen(wb)*8;
    fb_drawstring(W-wlen-30, sy+ST_H/2-4, wb, BODY_C, STATUS_BG);
    /* Search icon */
    draw_circle(W-14,sy+ST_H/2,6,ICON_C);
    vl(W-10,sy+ST_H/2+4,sy+ST_H/2+8,ICON_C);
}

/* ── Full render ── */
void render_doc(void){
    layout();
    fb_clear(BG);
    draw_left_panel();
    const char*title = (active_doc>=0) ? doc_store[active_doc].title : "Untitled";
    const char*body  = (active_doc>=0) ? doc_store[active_doc].body  : "";
    draw_tabbar(title);
    draw_fmt_toolbar();
    draw_doc_content(body, title);
    int wc=word_count(body);
    draw_status_bar(wc, current_page==PAGE_DOC_EDITOR?"insert":"read");
}

/* ── Fill content ── */
static void fill_doc(const char*topic, char t){
    if(active_doc<0||active_doc>=doc_count) return;
    char*b=doc_store[active_doc].body; b[0]=0;
    if(t=='E'){
        dcat(b,"Introduction\n\n",DOC_BODY_LEN);
        if(dstart(topic,"ai")||dstart(topic,"artificial")){
            dcat(b,"Artificial intelligence (AI) is one of the most transformative technologies of the 21st century. AI is integrated into healthcare, finance, education, and cybersecurity.\n\n",DOC_BODY_LEN);
        } else if(dstart(topic,"climate")){
            dcat(b,"Climate change represents the defining environmental challenge of our era. The scientific consensus confirms that global temperatures are rising due to anthropogenic greenhouse gas emissions. This essay examines the causes, consequences, and potential solutions to this critical issue.\n\n",DOC_BODY_LEN);
        } else if(dstart(topic,"congo")||dstart(topic,"africa")){
            dcat(b,"The Democratic Republic of Congo possesses extraordinary natural wealth and human capital. Strategic development of these resources through targeted investment and sound governance can transform the nation into a regional leader in the 21st century.\n\n",DOC_BODY_LEN);
        } else {
            dcat(b,"This essay examines the key dimensions of: ",DOC_BODY_LEN);
            dcat(b,topic,DOC_BODY_LEN);
            dcat(b,". Through careful analysis we illuminate the core insights and implications of this important subject.\n\n",DOC_BODY_LEN);
        }
        dcat(b,"Applications of AI\n\n",DOC_BODY_LEN);
        dcat(b,"Healthcare\n\n",DOC_BODY_LEN);
        dcat(b,"In the healthcare sector, AI is used to assist with diagnostics, predict patient outcomes, and personalize treatment plans. AI driven tools can analyze medical images like X-rays and MRIs with remarkable accuracy, aiding doctors in early and accurate disease detection.\n\n",DOC_BODY_LEN);
        dcat(b,"Education\n\n",DOC_BODY_LEN);
        dcat(b,"AI is reshaping education through personalized learning platforms, automated grading, and intelligent tutoring systems. These innovations enable educators to reach more students more effectively.\n\n",DOC_BODY_LEN);
        dcat(b,"Conclusion\n\n",DOC_BODY_LEN);
        dcat(b,"Artificial Intelligence represents both an immense opportunity and a significant responsibility. As we integrate AI deeper into society, careful governance and ethical frameworks must guide its development.\n\n",DOC_BODY_LEN);
        dcat(b,"References\n\n",DOC_BODY_LEN);
        dcat(b,"[1] AIMERANCIA Knowledge Base, 2026\n",DOC_BODY_LEN);
        dcat(b,"[2] International Research Database\n",DOC_BODY_LEN);
        dcat(b,"[3] Journal of AI and Society\n",DOC_BODY_LEN);
    } else if(t=='R'){
        dcat(b,"Executive Summary\n\n",DOC_BODY_LEN);
        dcat(b,"Subject: ",DOC_BODY_LEN); dcat(b,topic,DOC_BODY_LEN); dcat(b,"\n",DOC_BODY_LEN);
        dcat(b,"Prepared by: AIMERANCIA Intelligence System\n\n",DOC_BODY_LEN);
        dcat(b,"Comprehensive analysis indicates viable pathways with appropriate resource allocation. This report outlines key findings, recommendations, and an implementation timeline.\n\n",DOC_BODY_LEN);
        dcat(b,"Key Findings\n\n",DOC_BODY_LEN);
        dcat(b,"The analysis reveals three critical areas requiring immediate attention. First, resource optimization must be prioritized. Second, stakeholder alignment is essential. Third, technical infrastructure requires upgrading.\n\n",DOC_BODY_LEN);
        dcat(b,"Timeline\n\n",DOC_BODY_LEN);
        dcat(b,"Phase 1 Planning      3 months\nPhase 2 Development   6 months\nPhase 3 Testing       2 months\nPhase 4 Deployment    1 month\n\n",DOC_BODY_LEN);
        dcat(b,"Recommendations\n\n",DOC_BODY_LEN);
        dcat(b,"Proceed subject to stakeholder approval and budget confirmation.\n",DOC_BODY_LEN);
    } else if(t=='L'){
        dcat(b,"                    AIMERANCIA System\n\n",DOC_BODY_LEN);
        dcat(b,"To: ",DOC_BODY_LEN); dcat(b,topic,DOC_BODY_LEN); dcat(b,"\n\n",DOC_BODY_LEN);
        dcat(b,"Dear Sir or Madam\n\n",DOC_BODY_LEN);
        dcat(b,"I write to bring an important matter to your attention. After careful consideration and analysis, it is evident that immediate action is required to address the issues outlined below.\n\n",DOC_BODY_LEN);
        dcat(b,"I respectfully request your earliest consideration and look forward to your response.\n\n",DOC_BODY_LEN);
        dcat(b,"Yours faithfully\nAIMERANCIA Intelligence System\n",DOC_BODY_LEN);
    } else {
        dcat(b,"Project: ",DOC_BODY_LEN); dcat(b,topic,DOC_BODY_LEN); dcat(b,"\n\n",DOC_BODY_LEN);
        dcat(b,"Objectives\n\n",DOC_BODY_LEN);
        dcat(b,"Primary goal is successful and timely completion within budget. Secondary objective is to establish a repeatable process for future projects.\n\n",DOC_BODY_LEN);
        dcat(b,"Milestones\n\n",DOC_BODY_LEN);
        dcat(b,"M1 Initiation and team assembly\nM2 Requirements complete\nM3 Design approved\nM4 Implementation complete\nM5 Testing and quality assurance\nM6 Deployment and handover\n\n",DOC_BODY_LEN);
        dcat(b,"Budget\n\n",DOC_BODY_LEN);
        dcat(b,"Personnel 40 percent\nEquipment 25 percent\nOperations 20 percent\nContingency 15 percent\n",DOC_BODY_LEN);
    }
}

/* ── Public API ── */
void doc_page_init(void){
    doc_count=0; active_doc=-1; view_scroll=0;
    for(int i=0;i<DOC_STORE_MAX;i++) doc_store[i].used=0;
}

void doc_page_open_browser(void){
    current_page=PAGE_DOC_BROWSER;
    if(doc_count==0) active_doc=-1;
    else if(active_doc<0) active_doc=0;
    render_doc();
}

void doc_page_open_editor(const char*title,const char*type){
    if(doc_count>=DOC_STORE_MAX) doc_count=0;
    active_doc=doc_count;
    dcopy(doc_store[active_doc].title,title,DOC_TITLE_LEN);
    dcopy(doc_store[active_doc].type, type, 16);
    doc_store[active_doc].body[0]=0;
    doc_store[active_doc].used=1;
    doc_count++;
    view_scroll=0;
    current_page=PAGE_DOC_EDITOR;
    render_doc();
}

void doc_page_open_viewer(int index){
    if(index<0||index>=doc_count) return;
    active_doc=index;
    view_scroll=0;
    current_page=PAGE_DOC_VIEWER;
    render_doc();
}

void doc_page_close(void){
    current_page=PAGE_MAIN;
    extern void aios_ui_draw(void);
    aios_ui_draw();
}

void doc_page_write(const char*text){
    if(active_doc<0||active_doc>=doc_count) return;
    dcat(doc_store[active_doc].body,text,DOC_BODY_LEN);
    render_doc();
}

void doc_page_save(void){
    if(active_doc<0||active_doc>=doc_count) return;
    doc_entry_t*d=&doc_store[active_doc];
    char key[64]="doc_";
    int ki=4,ti=0;
    while(d->title[ti]&&ki<60){key[ki++]=d->title[ti++];}key[ki]=0;
    kb_set(key,d->body[0]?d->body:"(empty)");
    kbfs_save();
    render_doc();
}

void doc_page_tick(void){
    page_tick++;
    if(current_page==PAGE_DOC_EDITOR && page_tick%30==0)
        render_doc();
}

int doc_page_active(void){
    return current_page!=PAGE_MAIN;
}

int doc_page_handle(const char*input){
    /* Open document browser */
    if(deq(input,"open documents")||deq(input,"documents")||
       deq(input,"my documents")||deq(input,"mes documents")||
       deq(input,"doc browser")||deq(input,"fichiers")||
       deq(input,"access documents")){
        doc_page_open_browser(); return 1;
    }
    /* New blank doc */
    if(deq(input,"new document")||deq(input,"nouveau document")||
       deq(input,"create document")||deq(input,"creer document")){
        doc_page_open_editor("Untitled","Document"); return 1;
    }
    /* Long document detection — parse page count */
    {
        int pages=5; /* default */
        const char*ti=input;
        /* Look for page count: "40 page", "10 pages", "a 5 page" */
        const char*p=input;
        while(*p){
            if(*p>='1'&&*p<='9'){
                int n=0;
                while(*p>='0'&&*p<='9'){n=n*10+(*p-'0');p++;}
                while(*p==' ')p++;
                if(dstart(p,"page")||dstart(p,"pg")||dstart(p,"chapitre")){
                    if(n>=1&&n<=40) pages=n;
                }
            }
            p++;
        }

        /* Essay */
        if(dstart(input,"write essay about ")||dstart(input,"essay about ")||
           dstart(input,"essai sur ")||dstart(input,"redige essai")||
           dstart(input,"write a ")&&(dstart(input+8,"essay")||dstart(input+10,"essay")||dstart(input+11,"essay"))){
            const char*t=input;
            if(dstart(input,"write essay about "))t=input+18;
            else if(dstart(input,"essay about "))t=input+12;
            else if(dstart(input,"essai sur "))t=input+10;
            else if(dstart(input,"write a ")){
                /* skip "write a N page essay about " */
                t=input+8;
                while(*t&&*t!=' ')t++; /* skip number/article */
                while(*t==' ')t++;
                if(dstart(t,"page "))t+=5;
                if(dstart(t,"pages "))t+=6;
                if(dstart(t,"essay about "))t+=12;
                else if(dstart(t,"essay on "))t+=9;
                else if(dstart(t,"essay "))t+=6;
            }
            doc_page_open_editor(t,"Essay");
            doc_page_write_long(t,"Essay",t,pages);
            doc_page_save(); render_doc(); return 1;
        }
        /* Report */
        if(dstart(input,"write report")||dstart(input,"create report")||
           dstart(input,"genere rapport")||dstart(input,"rapport sur ")||
           dstart(input,"write a report")){
            const char*t="Report";
            if(dstart(input,"write report on "))t=input+16;
            else if(dstart(input,"write report about "))t=input+19;
            else if(dstart(input,"rapport sur "))t=input+12;
            else if(dstart(input,"write a report on "))t=input+18;
            else if(dstart(input,"write a report about "))t=input+21;
            doc_page_open_editor(t,"Report");
            doc_page_write_long(t,"Report",t,pages);
            doc_page_save(); render_doc(); return 1;
        }
        /* Assignment */
        if(dstart(input,"write assignment")||dstart(input,"assignment on ")||
           dstart(input,"assignment about ")||dstart(input,"write a assignment")){
            const char*t="Assignment";
            if(dstart(input,"assignment on "))t=input+14;
            else if(dstart(input,"assignment about "))t=input+17;
            else if(dstart(input,"write assignment on "))t=input+20;
            doc_page_open_editor(t,"Assignment");
            doc_page_write_long(t,"Assignment",t,pages);
            doc_page_save(); render_doc(); return 1;
        }
        /* Research paper */
        if(dstart(input,"write research paper")||dstart(input,"research paper on ")||
           dstart(input,"research paper about ")){
            const char*t="Research";
            if(dstart(input,"research paper on "))t=input+18;
            else if(dstart(input,"research paper about "))t=input+21;
            doc_page_open_editor(t,"Research Paper");
            doc_page_write_long(t,"Research Paper",t,pages);
            doc_page_save(); render_doc(); return 1;
        }
    }
    /* Letter */
    if(dstart(input,"write letter")||dstart(input,"compose letter")||
       dstart(input,"ecris lettre")){
        const char*t="Recipient";
        if(dstart(input,"write letter to "))t=input+16;
        else if(dstart(input,"ecris lettre a "))t=input+15;
        doc_page_open_editor(t,"Letter");
        fill_doc(t,'L'); doc_page_save(); render_doc(); return 1;
    }
    /* Plan */
    if(dstart(input,"create plan")||dstart(input,"write plan")||
       dstart(input,"project plan")||dstart(input,"plan pour ")){
        const char*t="New Project";
        if(dstart(input,"create plan for "))t=input+16;
        else if(dstart(input,"write plan for "))t=input+15;
        else if(dstart(input,"plan pour "))t=input+10;
        doc_page_open_editor(t,"Plan");
        fill_doc(t,'P'); doc_page_save(); render_doc(); return 1;
    }
    /* Open by number or name */
    if(dstart(input,"open doc ")||dstart(input,"read doc ")||
       dstart(input,"lire doc ")||dstart(input,"ouvre doc ")){
        const char*n=input+9;
        if(n[0]>='1'&&n[0]<='9'){
            doc_page_open_viewer(n[0]-'1'); return 1;
        }
        for(int i=0;i<doc_count;i++){
            const char*t=doc_store[i].title,*q=n; int m=1;
            while(*q&&*t){if(*q!=*t)m=0;q++;t++;}
            if(m){doc_page_open_viewer(i);return 1;}
        }
        return 1;
    }
    /* Scroll */
    if(deq(input,"scroll down")||deq(input,"page down")||deq(input,"suivant")){
        if(current_page!=PAGE_MAIN){view_scroll+=4;render_doc();}return 1;
    }
    if(deq(input,"scroll up")||deq(input,"page up")||deq(input,"precedent")){
        if(current_page!=PAGE_MAIN){if(view_scroll>0)view_scroll-=4;render_doc();}return 1;
    }
    /* Save / close */
    if(deq(input,"save")||deq(input,"sauvegarder")){
        if(current_page==PAGE_DOC_EDITOR){doc_page_save();}return 1;
    }
    if(deq(input,"close")||deq(input,"fermer")||deq(input,"back")||
       deq(input,"retour")||deq(input,"exit document")){
        doc_page_close(); return 1;
    }
    /* List */
    if(deq(input,"list documents")||deq(input,"liste documents")){
        terminal_print_color("[DOCS] ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(doc_count);
        terminal_print_color(" documents\n",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        for(int i=0;i<doc_count;i++){
            terminal_print_color("  ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_int(i+1);
            terminal_print_color(". ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
            terminal_print_color(doc_store[i].title,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
            terminal_newline();
        }
        if(current_page==PAGE_DOC_BROWSER) render_doc();
        return 1;
    }
    return 0;
}
