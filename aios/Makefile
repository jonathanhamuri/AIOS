all: aios.bin

boot/stage1.o: boot/boot.asm
	nasm -f bin boot/boot.asm -o boot/stage1.o

boot/stage2.o: boot/stage2.asm
	nasm -f bin boot/stage2.asm -o boot/stage2.o

kernel/entry.o: kernel/entry.asm
	nasm -f bin kernel/entry.asm -o kernel/entry.o

aios.bin: boot/stage1.o boot/stage2.o kernel/entry.o
	cat boot/stage1.o boot/stage2.o kernel/entry.o > aios.bin

run: aios.bin
	qemu-system-x86_64 -drive format=raw,file=aios.bin,index=0,media=disk -display none -serial stdio -no-reboot

clean:
	rm -f boot/stage1.o boot/stage2.o kernel/entry.o aios.bin
