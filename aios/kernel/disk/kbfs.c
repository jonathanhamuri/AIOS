#include "kbfs.h"
#include "ata.h"
#include "ai/knowledge/kb.h"
#include "terminal/terminal.h"

#define KBFS_MAGIC    0x41494F53
#define SECTOR_HEADER 10
#define SECTOR_FACTS  11
#define SECTOR_CMDS   75

typedef struct {
    unsigned int magic;
    unsigned int active;
    char key[64];
    char val[256];
} __attribute__((packed)) fact_sector_t;

typedef struct {
    unsigned int magic;
    unsigned int active;
    char trigger[64];
    char source[256];
} __attribute__((packed)) cmd_sector_t;

static void scopy(char*d,const char*s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}

void kbfs_save() {
    unsigned char buf[512];
    int i, count=0;
    for(i=0;i<512;i++) buf[i]=0;
    ((unsigned int*)buf)[0] = KBFS_MAGIC;
    // Debug: read status before write
    unsigned char st;
    __asm__ volatile("inb $0x1F7,%0":"=a"(st));
    terminal_print_color("[ATA] status=0x",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_int(st);
    terminal_newline();
    int hdr_ok = ata_write_sector(SECTOR_HEADER, buf);
    if(hdr_ok < 0){
        __asm__ volatile("inb $0x1F7,%0":"=a"(st));
        terminal_print_color("[KBFS] FAIL status=0x",MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        terminal_print_int(st);
        terminal_newline();
        return;
    }
    terminal_print_color("[KBFS] Header written OK\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));

    for(i=0;i<128;i++){
        for(int j=0;j<512;j++) buf[j]=0;
        fact_sector_t* fs=(fact_sector_t*)buf;
        const char* key=kb_get_key(i);
        const char* val=kb_get_val(i);
        int active=kb_get_active(i);
        fs->magic=KBFS_MAGIC;
        if(active&&key&&val){
            fs->active=1;
            scopy(fs->key,key,64);
            scopy(fs->val,val,256);
            count++;
        }
        ata_write_sector(SECTOR_FACTS+i, buf);
    }

    for(i=0;i<32;i++){
        for(int j=0;j<512;j++) buf[j]=0;
        cmd_sector_t* cs=(cmd_sector_t*)buf;
        const char* trigger=commands_get_trigger(i);
        const char* source=commands_get_source(i);
        int active=commands_get_active(i);
        cs->magic=KBFS_MAGIC;
        if(active&&trigger&&source){
            cs->active=1;
            scopy(cs->trigger,trigger,64);
            scopy(cs->source,source,256);
        }
        ata_write_sector(SECTOR_CMDS+i, buf);
    }

    terminal_print_color("[AIOS] Saved: ", MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print_int(count);
    terminal_print_color(" facts to disk\n", MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
}

void kbfs_load() {
    unsigned char buf[512];
    if(ata_read_sector(SECTOR_HEADER, buf)<0) return;
    if(((unsigned int*)buf)[0]!=KBFS_MAGIC) return;

    int loaded=0;
    for(int i=0;i<128;i++){
        if(ata_read_sector(SECTOR_FACTS+i, buf)<0) continue;
        fact_sector_t* fs=(fact_sector_t*)buf;
        if(fs->magic!=KBFS_MAGIC||!fs->active) continue;
        kb_set(fs->key, fs->val);
        loaded++;
    }
    for(int i=0;i<32;i++){
        if(ata_read_sector(SECTOR_CMDS+i, buf)<0) continue;
        cmd_sector_t* cs=(cmd_sector_t*)buf;
        if(cs->magic!=KBFS_MAGIC||!cs->active) continue;
        kb_learn(cs->trigger, cs->source);
    }
    if(loaded>0){
        terminal_print_color("[AIOS] Loaded: ", MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(loaded);
        terminal_print_color(" facts from disk\n", MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    }
}

void kbfs_init() {
    terminal_print_color("KBFS             : OK\n", MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}
