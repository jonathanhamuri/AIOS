#ifndef ATA_H
#define ATA_H

// ATA PIO mode - raw sector read/write (512 bytes per sector)
#define ATA_SECTOR_SIZE 512

void ata_init();
int  ata_read_sector (unsigned int lba, unsigned char* buf);
int  ata_write_sector(unsigned int lba, unsigned char* buf);

#endif
