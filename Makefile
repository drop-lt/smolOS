all: run

smolOS.bin: build/bootloader.o build/main.o
	x86_64-elf-ld -m elf_i386 -o $@ -Ttext 0x7c00 $^ --oformat binary

run: smolOS.bin
	qemu-system-x86_64 -hda $<

build/main.o: main.c 
	x86_64-elf-gcc -Wall -Wextra -m32 -nostdlib -ffreestanding -c $< -o $@ -Os -mno-red-zone -mno-mmx -mno-sse -mno-sse2
	objcopy --remove-section=.note.gnu.property $@ -x

build/bootloader.o: bootloader.asm
	nasm $< -f elf -o $@
	objcopy --remove-section=.note.gnu.property $@ -x

clean:
	$(RM) *.bin build/*.o 