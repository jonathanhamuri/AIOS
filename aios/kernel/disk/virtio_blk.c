#include "virtio_blk.h"
#include "ata.h"

int virtio_blk_ready = 0;

static unsigned int pci_read(unsigned char bus, unsigned char dev,
                              unsigned char fn, unsigned char reg){
    unsigned int addr=(1u<<31)|((unsigned int)bus<<16)|
                      ((unsigned int)dev<<11)|((unsigned int)fn<<8)|(reg&0xFC);
    __asm__ volatile("outl %0,%%dx"::"a"(addr),"d"((unsigned short)0xCF8));
    unsigned int val;
    __asm__ volatile("inl %%dx,%0":"=a"(val):"d"((unsigned short)0xCFC));
    return val;
}

int virtio_blk_init(void){
    for(int bus=0;bus<4;bus++){
        for(int dev=0;dev<32;dev++){
            unsigned int id=pci_read(bus,dev,0,0);
            if((id&0xFFFF)==0x1AF4 && ((id>>16)&0xFFFF)==0x1001){
                virtio_blk_ready=1;
                return 0;
            }
        }
    }
    return -1;
}

int virtio_blk_read(unsigned int sector, void* buf){
    return ata_read_sector(sector,(unsigned char*)buf);
}

int virtio_blk_write(unsigned int sector, const void* buf){
    return ata_write_sector(sector,(unsigned char*)buf);
}
