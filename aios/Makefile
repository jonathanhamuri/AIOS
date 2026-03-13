CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostdinc -O0 -fno-asynchronous-unwind-tables -Ikernel

all: aios.iso

kernel/entry_mb.o: kernel/entry_mb.asm
	nasm -f elf32 kernel/entry_mb.asm -o kernel/entry_mb.o

kernel/kernel_main.o: kernel/kernel_main.c
	$(CC) $(CFLAGS) -c kernel/kernel_main.c -o kernel/kernel_main.o

kernel/mm/pmm.o: kernel/mm/pmm.c
	$(CC) $(CFLAGS) -c kernel/mm/pmm.c -o kernel/mm/pmm.o

kernel/mm/heap.o: kernel/mm/heap.c
	$(CC) $(CFLAGS) -c kernel/mm/heap.c -o kernel/mm/heap.o

iso/boot/kernel.elf: kernel/entry_mb.o kernel/kernel_main.o kernel/mm/pmm.o kernel/mm/heap.o
	ld -m elf_i386 -T kernel/linker_mb.ld -o iso/boot/kernel.elf kernel/entry_mb.o kernel/kernel_main.o kernel/mm/pmm.o kernel/mm/heap.o

aios.iso: iso/boot/kernel.elf
	grub-mkrescue -o aios.iso iso/ 2>/dev/null

run: aios.iso
	qemu-system-x86_64 -cdrom aios.iso -display curses -no-reboot

clean:
	rm -f kernel/entry_mb.o kernel/kernel_main.o kernel/mm/pmm.o kernel/mm/heap.o iso/boot/kernel.elf aios.iso
