#include "docgen.h"
#include "../../terminal/terminal.h"

static int sstart(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static int seq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}

static document_t current_doc;

static void doc_add(document_t*d,const char*line){
    if(d->line_count>=DOC_MAX_LINES)return;
    scopy(d->lines[d->line_count],line,DOC_LINE_LEN);
    d->line_count++;
}

static void print_doc(document_t*d){
    terminal_print_color("============================================================\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color("  ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(d->title,MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_newline();
    terminal_print_color("============================================================\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    for(int i=0;i<d->line_count;i++){
        if(!d->lines[i][0]){terminal_newline();continue;}
        if(d->lines[i][0]=='#'){
            terminal_print_color(d->lines[i]+1,MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        } else {
            terminal_print_color(d->lines[i],MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        }
        terminal_newline();
    }
    terminal_print_color("============================================================\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void docgen_write_essay(const char*topic,const char*style){
    document_t*d=&current_doc;
    d->line_count=0;
    scopy(d->title,topic,64);
    scopy(d->type,"Essay",32);
    doc_add(d,"#  ACADEMIC ESSAY");
    doc_add(d,"");
    doc_add(d,"# I. INTRODUCTION");
    doc_add(d,"");
    if(sstart(topic,"climate")){
        doc_add(d,"  Climate change is one of the most critical");
        doc_add(d,"  challenges of the 21st century. Scientific");
        doc_add(d,"  evidence confirms rising global temperatures");
        doc_add(d,"  driven by greenhouse gas emissions.");
    } else if(sstart(topic,"ai")||sstart(topic,"artificial")){
        doc_add(d,"  Artificial Intelligence is transforming every");
        doc_add(d,"  sector of human society. From healthcare to");
        doc_add(d,"  education, AI systems are enabling new levels");
        doc_add(d,"  of efficiency and capability.");
    } else if(sstart(topic,"congo")||sstart(topic,"africa")){
        doc_add(d,"  The Democratic Republic of Congo possesses");
        doc_add(d,"  extraordinary natural wealth and human capital.");
        doc_add(d,"  Strategic development of these resources can");
        doc_add(d,"  transform the nation into a regional leader.");
    } else {
        doc_add(d,"  This essay examines the key dimensions of");
        char line[60]="  the topic: ";
        int i=13,j=0;
        while(topic[j]&&i<58){line[i++]=topic[j++];}line[i]=0;
        doc_add(d,line);
        doc_add(d,"  Through analysis we illuminate core insights.");
    }
    doc_add(d,"");
    doc_add(d,"# II. ANALYSIS");
    doc_add(d,"");
    doc_add(d,"  A systematic analysis reveals key dimensions:");
    doc_add(d,"  1. Economic and resource implications");
    doc_add(d,"  2. Social and cultural impact");
    doc_add(d,"  3. Technological considerations");
    doc_add(d,"  4. Policy and governance frameworks");
    doc_add(d,"");
    doc_add(d,"# III. SOLUTIONS");
    doc_add(d,"");
    doc_add(d,"  Comprehensive policy frameworks must be");
    doc_add(d,"  established. International cooperation should");
    doc_add(d,"  be prioritized alongside investment in R&D.");
    doc_add(d,"");
    doc_add(d,"# IV. CONCLUSION");
    doc_add(d,"");
    doc_add(d,"  This essay demonstrates the need for a");
    doc_add(d,"  multidisciplinary approach combining policy,");
    doc_add(d,"  technology and social innovation.");
    doc_add(d,"");
    doc_add(d,"# REFERENCES");
    doc_add(d,"  [1] AIOS Knowledge Base, 2026");
    doc_add(d,"  [2] International Research Database");
    doc_add(d,"  [3] Academic Journal of Technology");
    print_doc(d);
}

void docgen_write_report(const char*topic){
    document_t*d=&current_doc;
    d->line_count=0;
    scopy(d->title,topic,64);
    scopy(d->type,"Report",32);
    doc_add(d,"#  TECHNICAL REPORT");
    doc_add(d,"");
    char tl[60]="  Subject: ";int i=11,j=0;
    while(topic[j]&&i<58){tl[i++]=topic[j++];}tl[i]=0;
    doc_add(d,tl);
    doc_add(d,"  By: AIOS Engineering Intelligence");
    doc_add(d,"");
    doc_add(d,"# 1. EXECUTIVE SUMMARY");
    doc_add(d,"");
    doc_add(d,"  Comprehensive analysis indicates viable");
    doc_add(d,"  pathways with appropriate resource allocation.");
    doc_add(d,"");
    doc_add(d,"# 2. TIMELINE");
    doc_add(d,"  Phase 1: Planning     (3 months)");
    doc_add(d,"  Phase 2: Development  (6 months)");
    doc_add(d,"  Phase 3: Testing      (2 months)");
    doc_add(d,"  Phase 4: Deployment   (1 month)");
    doc_add(d,"");
    doc_add(d,"# 3. RECOMMENDATIONS");
    doc_add(d,"  Proceed subject to stakeholder approval.");
    print_doc(d);
}

void docgen_write_letter(const char*to,const char*subject){
    document_t*d=&current_doc;
    d->line_count=0;
    scopy(d->title,subject,64);
    scopy(d->type,"Letter",32);
    doc_add(d,"                    AIOS System");
    doc_add(d,"                    Network: 10.0.2.15");
    doc_add(d,"");
    char tl[60]="  To: ";int i=6,j=0;
    while(to[j]&&i<58){tl[i++]=to[j++];}tl[i]=0;
    doc_add(d,tl);
    char sl[60]="  Re: ";i=6;j=0;
    while(subject[j]&&i<58){sl[i++]=subject[j++];}sl[i]=0;
    doc_add(d,sl);
    doc_add(d,"");
    doc_add(d,"  Dear Sir/Madam,");
    doc_add(d,"");
    doc_add(d,"  I write regarding the above matter.");
    doc_add(d,"  After careful analysis, immediate action");
    doc_add(d,"  is required. I respectfully request your");
    doc_add(d,"  consideration at your earliest convenience.");
    doc_add(d,"");
    doc_add(d,"  Yours faithfully,");
    doc_add(d,"  AIOS Intelligence System");
    print_doc(d);
}

void docgen_write_plan(const char*project){
    document_t*d=&current_doc;
    d->line_count=0;
    scopy(d->title,project,64);
    scopy(d->type,"Plan",32);
    doc_add(d,"#  PROJECT PLAN");
    doc_add(d,"");
    char tl[60]="  Project: ";int i=11,j=0;
    while(project[j]&&i<58){tl[i++]=project[j++];}tl[i]=0;
    doc_add(d,tl);
    doc_add(d,"");
    doc_add(d,"# OBJECTIVES");
    doc_add(d,"  Primary:   Successful completion");
    doc_add(d,"  Secondary: On-time and within budget");
    doc_add(d,"");
    doc_add(d,"# MILESTONES");
    doc_add(d,"  M1: Initiation and team assembly");
    doc_add(d,"  M2: Requirements complete");
    doc_add(d,"  M3: Design approved");
    doc_add(d,"  M4: Implementation complete");
    doc_add(d,"  M5: Testing and QA");
    doc_add(d,"  M6: Deployment");
    doc_add(d,"");
    doc_add(d,"# BUDGET");
    doc_add(d,"  Personnel:   40%");
    doc_add(d,"  Equipment:   25%");
    doc_add(d,"  Operations:  20%");
    doc_add(d,"  Contingency: 15%");
    print_doc(d);
}

int docgen_handle(const char*input){
    if(sstart(input,"write essay about ")||sstart(input,"essay about ")||sstart(input,"essay on ")){
        const char*t="General Topic";
        if(sstart(input,"write essay about "))t=input+18;
        else if(sstart(input,"essay about "))t=input+12;
        else if(sstart(input,"essay on "))t=input+9;
        docgen_write_essay(t,"academic");return 1;
    }
    if(sstart(input,"write report")||sstart(input,"create report")){
        const char*t="General";
        if(sstart(input,"write report on "))t=input+16;
        else if(sstart(input,"write report about "))t=input+19;
        else if(sstart(input,"create report on "))t=input+17;
        docgen_write_report(t);return 1;
    }
    if(sstart(input,"write letter")||sstart(input,"compose letter")){
        const char*to="Recipient";const char*sub="Important Matter";
        if(sstart(input,"write letter to ")){
            to=input+16;
            const char*p=to;
            while(*p&&!sstart(p," about "))p++;
            if(sstart(p," about "))sub=p+7;
        }
        docgen_write_letter(to,sub);return 1;
    }
    if(sstart(input,"create plan")||sstart(input,"write plan")||sstart(input,"project plan")){
        const char*p="New Project";
        if(sstart(input,"create plan for "))p=input+16;
        else if(sstart(input,"write plan for "))p=input+15;
        else if(sstart(input,"project plan for "))p=input+17;
        docgen_write_plan(p);return 1;
    }
    if(seq(input,"documents")||seq(input,"doc help")){
        terminal_print_color("[DOC] Commands:\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print("  write essay about <topic>\n");
        terminal_print("  write report on <topic>\n");
        terminal_print("  write letter to <name> about <subject>\n");
        terminal_print("  create plan for <project>\n");
        return 1;
    }
    return 0;
}
