CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostdinc -O0 -fno-asynchronous-unwind-tables -Ikernel

OBJS = kernel/entry_mb.o \
       kernel/kernel_main.o \
       kernel/mm/pmm.o \
       kernel/mm/heap.o \
       kernel/terminal/terminal.o \
       kernel/terminal/ai_input.o \
       kernel/syscall/syscall.o \
       kernel/syscall/syscall_asm.o

all: aios.iso

kernel/entry_mb.o: kernel/entry_mb.asm
	nasm -f elf32 kernel/entry_mb.asm -o kernel/entry_mb.o

kernel/syscall/syscall_asm.o: kernel/syscall/syscall_asm.asm
	nasm -f elf32 kernel/syscall/syscall_asm.asm -o kernel/syscall/syscall_asm.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

iso/boot/kernel.elf: $(OBJS)
	ld -m elf_i386 -T kernel/linker_mb.ld -o iso/boot/kernel.elf $(OBJS)

aios.iso: iso/boot/kernel.elf
	grub-mkrescue -o aios.iso iso/ 2>/dev/null

run: aios.iso
	qemu-system-x86_64 -cdrom aios.iso -display curses -no-reboot

clean:
	rm -f $(OBJS) iso/boot/kernel.elf aios.iso
