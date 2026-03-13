#include "ata.h"
#include "terminal/terminal.h"

#define ATA_DATA        0x1F0
#define ATA_SECT_COUNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7
#define ATA_ALT         0x3F6
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_SR_BSY      0x80
#define ATA_SR_DRQ      0x08
#define ATA_SR_ERR      0x01

static void outb(unsigned short p,unsigned char v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static void outw(unsigned short p,unsigned short v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static unsigned char  inb(unsigned short p){unsigned char v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static unsigned short inw(unsigned short p){unsigned short v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}

static void delay400(){ inb(ATA_ALT);inb(ATA_ALT);inb(ATA_ALT);inb(ATA_ALT); }

static int poll(int check_drq){
    int t=100000;
    while(t--){
        unsigned char s=inb(ATA_STATUS);
        if(s&ATA_SR_ERR) return -1;
        if(s&ATA_SR_BSY) continue;
        if(!check_drq) return 0;
        if(s&ATA_SR_DRQ) return 0;
    }
    return -1;
}

void ata_init(){
    unsigned char p=inb(0x1F7), s=inb(0x177);
    if(p!=0xFF) terminal_print_color("[ATA] Primary OK\n",  MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    if(s!=0xFF) terminal_print_color("[ATA] Secondary OK\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    outb(0x376,0x04); delay400(); outb(0x376,0x00); delay400();
    outb(ATA_DRIVE,0xB0); delay400();
    terminal_print_color("Disk driver      : OK\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

int ata_read_sector(unsigned int lba, unsigned char* buf){
    if(poll(0)<0) return -1;
    outb(ATA_DRIVE, 0xF0|((lba>>24)&0x0F)); delay400();
    outb(ATA_SECT_COUNT, 1);
    outb(ATA_LBA_LO,  lba&0xFF);
    outb(ATA_LBA_MID, (lba>>8)&0xFF);
    outb(ATA_LBA_HI,  (lba>>16)&0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);
    if(poll(1)<0) return -1;
    for(int i=0;i<256;i++){
        unsigned short w=inw(ATA_DATA);
        buf[i*2]=w&0xFF; buf[i*2+1]=(w>>8)&0xFF;
    }
    return 0;
}

int ata_write_sector(unsigned int lba, unsigned char* buf){
    if(poll(0)<0) return -1;
    outb(ATA_DRIVE, 0xF0|((lba>>24)&0x0F)); delay400();
    outb(ATA_SECT_COUNT, 1);
    outb(ATA_LBA_LO,  lba&0xFF);
    outb(ATA_LBA_MID, (lba>>8)&0xFF);
    outb(ATA_LBA_HI,  (lba>>16)&0xFF);
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    if(poll(1)<0) return -1;
    for(int i=0;i<256;i++){
        unsigned short w=buf[i*2]|((unsigned short)buf[i*2+1]<<8);
        outw(ATA_DATA,w);
    }
    // flush
    outb(ATA_COMMAND,0xE7);
    poll(0);
    return 0;
}
