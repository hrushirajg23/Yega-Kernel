.PHONY: all check clean

all: yegaos.bin check

yegaos.bin: boot.o kernel.o
	i686-elf-gcc -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

boot.o: boot.asm
	nasm -f elf32 $< -o $@

kernel.o: kernel.c
	i686-elf-gcc -ffreestanding -c $< -o $@

check:
	@grub-file --is-x86-multiboot yegaos.bin && \
	echo "[OK] Multiboot compliant" || \
	(echo "[FAIL] Not Multiboot compliant" && exit 1)

clean:
	rm -f *.o *.bin
