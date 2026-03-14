#include "intent.h"

// ── string helpers ────────────────────────────────────────────────────
static int sl(const char* s){int n=0;while(*s++)n++;return n;}
static int seq(const char* a,const char* b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int sstart(const char* s,const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int scontains(const char* s,const char* p){
    int sl2=sl(p);
    for(int i=0;s[i];i++){
        int m=1;
        for(int j=0;j<sl2;j++)if(s[i+j]!=p[j]){m=0;break;}
        if(m)return 1;
    }
    return 0;
}
static void scopy(char* d,const char* s,int max){
    int i=0;while(s[i]&&i<max-1){d[i]=s[i];i++;}d[i]=0;
}
static void slower(char* d,const char* s,int max){
    int i=0;
    while(s[i]&&i<max-1){
        char c=s[i];
        if(c>='A'&&c<='Z')c+=32;
        d[i]=c;i++;
    }
    d[i]=0;
}

// Skip past first word, return pointer to rest
static const char* skip_word(const char* s){
    while(*s&&*s!=' ')s++;
    while(*s==' ')s++;
    return s;
}

// Extract quoted string or rest of line
static void extract_arg(const char* s, char* out, int max){
    while(*s==' ')s++;
    if(*s=='"'||*s==39){
        s++;
        int i=0;
        while(*s&&*s!='"'&&*s!=39&&i<max-1)out[i++]=*s++;
        out[i]=0;
    } else {
        scopy(out,s,max);
    }
}

// ── intent detection ─────────────────────────────────────────────────
void intent_parse(const char* input, intent_t* out){
    // Lowercase copy for matching
    char low[256];
    slower(low, input, 256);

    out->type       = INTENT_UNKNOWN;
    out->arg1[0]    = 0;
    out->arg2[0]    = 0;
    out->num1       = 0;
    out->confidence = 0;

    // Detect language
    if(scontains(low,"bonjour")||scontains(low,"affiche")||
       scontains(low,"montre")||scontains(low,"efface")||
       scontains(low,"memoire")||scontains(low,"aide")||
       scontains(low,"calcule")||scontains(low,"processus")||
       scontains(low,"salut")||scontains(low,"pourquoi")||
       scontains(low,"qu est")||scontains(low,"comment")){
        scopy(out->lang,"fr",4);
    } else {
        scopy(out->lang,"en",4);
    }

    // ── PRINT / AFFICHE ───────────────────────────────────────────
    if(sstart(low,"print ")||sstart(low,"show ")||
       sstart(low,"display ")||sstart(low,"say ")||
       sstart(low,"write ")||sstart(low,"output ")){
        out->type = INTENT_PRINT;
        extract_arg(skip_word(input), out->arg1, 128);
        out->confidence = 95;
        return;
    }
    if(sstart(low,"affiche ")||sstart(low,"montre ")||
       sstart(low,"ecris ")||sstart(low,"dis ")||
       sstart(low,"afficher ")){
        out->type = INTENT_PRINT;
        extract_arg(skip_word(input), out->arg1, 128);
        out->confidence = 95;
        return;
    }

    // ── GREET ─────────────────────────────────────────────────────
    if(sstart(low,"hello")||sstart(low,"hi ")||seq(low,"hi")||
       sstart(low,"hey")||sstart(low,"greet")){
        out->type = INTENT_GREET;
        scopy(out->lang,"en",4);
        out->confidence = 99;
        return;
    }
    if(sstart(low,"bonjour")||sstart(low,"salut")||sstart(low,"bonsoir")){
        out->type = INTENT_GREET;
        scopy(out->lang,"fr",4);
        out->confidence = 99;
        return;
    }

    // ── CLEAR ─────────────────────────────────────────────────────
    if(seq(low,"clear")||sstart(low,"clear ")||
       seq(low,"cls")||sstart(low,"efface")||sstart(low,"nettoie")){
        out->type = INTENT_CLEAR;
        out->confidence = 99;
        return;
    }

    // ── MEMORY ────────────────────────────────────────────────────
    if(scontains(low,"memory")||scontains(low,"memoire")||
       scontains(low,"ram")||scontains(low,"heap")||
       scontains(low,"free pages")||scontains(low,"pages libres")){
        out->type = INTENT_MEMORY;
        out->confidence = 90;
        return;
    }

    // ── HELP ──────────────────────────────────────────────────────
    if(seq(low,"help")||sstart(low,"help ")||
       seq(low,"aide")||sstart(low,"aide ")||
       scontains(low,"what can")||scontains(low,"que peux")){
        out->type = INTENT_HELP;
        out->confidence = 95;
        return;
    }

    // ── ABOUT ─────────────────────────────────────────────────────
    if(scontains(low,"about")||scontains(low,"what is aios")||
       scontains(low,"qu est ce")||scontains(low,"apropos")||
       scontains(low,"what are you")||scontains(low,"qui es tu")){
        out->type = INTENT_ABOUT;
        out->confidence = 90;
        return;
    }

    // ── CALCULATE ─────────────────────────────────────────────────
    if(sstart(low,"calc ")||sstart(low,"calculate ")||
       sstart(low,"calcule ")||sstart(low,"compute ")){
        out->type = INTENT_CALC;
        extract_arg(skip_word(input), out->arg1, 128);
        out->confidence = 90;
        return;
    }

    // ── PROCESS LIST ──────────────────────────────────────────────
    if(seq(low,"ps")||scontains(low,"processes")||
       scontains(low,"processus")||scontains(low,"list proc")){
        out->type = INTENT_LIST_PROC;
        out->confidence = 95;
        return;
    }

    // ── COMPILE / RUN ─────────────────────────────────────────────
    if(sstart(low,"compile ")||sstart(low,"run ")||
       sstart(low,"execute ")||sstart(low,"exec ")){
        out->type = INTENT_COMPILE;
        extract_arg(skip_word(input), out->arg1, 128);
        out->confidence = 90;
        return;
    }

    // ── AI QUERY (catch-all for natural questions) ─────────────────
    if(sstart(low,"what ")||sstart(low,"why ")||sstart(low,"how ")||
       sstart(low,"who ")||sstart(low,"when ")||sstart(low,"where ")||
       sstart(low,"pourquoi ")||sstart(low,"comment ")||
       sstart(low,"quand ")||sstart(low,"ou ")||sstart(low,"quoi ")){
        out->type = INTENT_AI_QUERY;
        scopy(out->arg1, input, 128);
        out->confidence = 70;
        return;
    }

    // Unknown
    out->type = INTENT_UNKNOWN;
    out->confidence = 0;
}

const char* intent_name(intent_type_t t){
    switch(t){
        case INTENT_PRINT:     return "PRINT";
        case INTENT_CLEAR:     return "CLEAR";
        case INTENT_MEMORY:    return "MEMORY";
        case INTENT_HELP:      return "HELP";
        case INTENT_ABOUT:     return "ABOUT";
        case INTENT_COMPILE:   return "COMPILE";
        case INTENT_CALC:      return "CALC";
        case INTENT_GREET:     return "GREET";
        case INTENT_LIST_PROC: return "LIST_PROC";
        case INTENT_AI_QUERY:  return "AI_QUERY";
        default:               return "UNKNOWN";
    }
}
