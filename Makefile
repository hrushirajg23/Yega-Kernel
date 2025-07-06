# === Toolchain ===
CC        = i686-elf-gcc
OBJCOPY   = i686-elf-objcopy
NASM      = nasm
CFLAGS    = -ffreestanding -O0 -g -nostdlib
INCLUDES  = -Iinclude
LDFLAGS   = -T linker/linker.ld

# === Build dirs ===
BUILD_DIR = build
ISO_DIR   = iso

# === Sources & Objects ===
KERNEL_SRC      = kernel/kernel.c
BOOT_SRC        = boot/boot.asm
GDT_C_SRC       = gdt/gdt.c
GDT_ASM_SRC     = gdt/gdt_flush.asm
IDT_SRC         = idt/idt.c
ISR_ASM_SRC     = idt/isr/isr.asm
EXCEPTIONS_SRC  = idt/isr/exceptions.c
PIC_SRC         = idt/pic/pic.c
SERIAL_SRC      = drivers/serial.c

OBJS = \
	$(BUILD_DIR)/boot.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/gdt_flush.o \
	$(BUILD_DIR)/idt.o \
	$(BUILD_DIR)/isr_asm.o \
	$(BUILD_DIR)/exceptions.o \
	$(BUILD_DIR)/pic.o \
	$(BUILD_DIR)/serial.o

# === Default ===
all: $(BUILD_DIR)/yegaos.iso check

# === Create dirs ===
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# === Assemble ===
$(BUILD_DIR)/boot.o: $(BOOT_SRC) | $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@

$(BUILD_DIR)/gdt_flush.o: $(GDT_ASM_SRC) | $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@

$(BUILD_DIR)/isr_asm.o: $(ISR_ASM_SRC) | $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@

# === Compile C ===
$(BUILD_DIR)/%.o: kernel/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(GDT_C_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/idt.o: $(IDT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/exceptions.o: $(EXCEPTIONS_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/pic.o: $(PIC_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/serial.o: $(SERIAL_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# === Link ELF ===
$(BUILD_DIR)/yegaos.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $^ -lgcc

# === Flat binary ===
$(BUILD_DIR)/yegaos.bin: $(BUILD_DIR)/yegaos.elf
	$(OBJCOPY) -O binary $< $@

# === Build ISO (now depends on both ELF and BIN) ===
$(BUILD_DIR)/yegaos.iso: \
    $(BUILD_DIR)/yegaos.elf \
    $(BUILD_DIR)/yegaos.bin \
    grub/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/yegaos.elf  $(ISO_DIR)/boot/yegaos.elf
	cp $(BUILD_DIR)/yegaos.bin  $(ISO_DIR)/boot/yegaos.bin
	cp grub/grub.cfg           $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $@ $(ISO_DIR) >/dev/null 2>&1

# === Multiboot check ===
check:
	@grub-file --is-x86-multiboot $(BUILD_DIR)/yegaos.elf && \
	  echo "[OK] Multiboot compliant" || \
	  (echo "[FAIL] Not Multiboot compliant" && exit 1)

# === Clean ===
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)

.PHONY: all check clean
