w#ifndef HEAP_H
#define HEAP_H

void  heap_init();
void* kmalloc(unsigned int size);
void  kfree(void* ptr);

#endif
