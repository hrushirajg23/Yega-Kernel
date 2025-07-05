BUILD_DIR = build
KERNEL_SRC = kernel/kernel.c
BOOT_SRC = boot/boot.asm
ISO_DIR = iso

all: $(BUILD_DIR)/yegaos.iso check

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# boot.asm
$(BUILD_DIR)/boot.o: $(BOOT_SRC) | $(BUILD_DIR)
	nasm -f elf32 $< -o $@

# kernel.c
$(BUILD_DIR)/kernel.o: $(KERNEL_SRC) | $(BUILD_DIR)
	i686-elf-gcc -ffreestanding -c $< -o $@

# gdt.c
$(BUILD_DIR)/gdt.o: gdt/gdt.c | $(BUILD_DIR)
	i686-elf-gcc -ffreestanding -c $< -o $@

# gdt.asm
$(BUILD_DIR)/gdt_asm.o: gdt/gdt.asm | $(BUILD_DIR)
	nasm -f elf32 $< -o $@

# idt.c
$(BUILD_DIR)/idt.o: idt/idt.c
	i686-elf-gcc -ffreestanding -c $< -o $@

# isr.asm
$(BUILD_DIR)/isr_asm.o: idt/isr/isr.asm 
	nasm -f elf32 $< -o $@

# isr - exceptions
$(BUILD_DIR)/exceptions.o: idt/isr/exceptions.c
	i686-elf-gcc -ffreestanding -c $< -o $@

# linking 
$(BUILD_DIR)/yegaos.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt_asm.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/isr_asm.o $(BUILD_DIR)/exceptions.o | $(BUILD_DIR)
	i686-elf-gcc -T linker/linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

# iso
$(BUILD_DIR)/yegaos.iso: $(BUILD_DIR)/yegaos.bin grub/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/yegaos.bin $(ISO_DIR)/boot/
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $@ $(ISO_DIR)

check:
	@grub-file --is-x86-multiboot $(BUILD_DIR)/yegaos.bin && \
	echo "[OK] Multiboot compliant" || \
	(echo "[FAIL] Not Multiboot compliant" && exit 1)

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)/boot/yegaos.bin $(ISO_DIR)/boot/grub/grub.cfg

.PHONY: all check clean
