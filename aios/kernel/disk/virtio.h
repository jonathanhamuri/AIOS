#ifndef VIRTIO_H
#define VIRTIO_H
void virtio_blk_init();
int  virtio_blk_read (unsigned int sector, unsigned char* buf);
int  virtio_blk_write(unsigned int sector, const unsigned char* buf);
int  virtio_blk_ready();
#endif
