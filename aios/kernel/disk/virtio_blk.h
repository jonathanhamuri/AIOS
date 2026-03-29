#ifndef VIRTIO_BLK_H
#define VIRTIO_BLK_H

int  virtio_blk_init(void);
int  virtio_blk_read(unsigned int sector, void* buf);
int  virtio_blk_write(unsigned int sector, const void* buf);
extern int virtio_blk_ready;

#endif
