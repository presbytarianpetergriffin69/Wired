# ==============
# WIRED MAKEFILE
# ==============

KERNDIR = kernel
DRIVDIR = drivers
IDIR    = include/sys
AMDDIR  = arch/amd64
AMDCDIR = $(AMDDIR)/cpu
AMDKDIR = $(AMDDIR)/klib
ISODIR  = IMGROOT
ISONAME = wiredos

CC      = x86_64-elf-gcc
ASM     = nasm -felf64
LD      = x86_64-elf-ld

LDFLAGS += \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-T linker.ld
IFLAGS  += -I$(IDIR)
CFLAGS  += \
	-std=gnu99 \
	-ffreestanding \
	-O2 \
	-Wall \
	-mno-red-zone \
	-mno-sse \
	-mno-sse2 \
	-m64 \
	-march=x86-64 \
	-mcmodel=kernel \
	-nostdlib \
	-Wextra

OBJ +=	obj/$(KERNDIR)/boot.o \
	obj/$(DRIVDIR)/console.o \
	obj/$(DRIVDIR)/font8x16.o \
	obj/$(DRIVDIR)/serial.o \
	obj/$(DRIVDIR)/crashsound.o \
	obj/$(AMDCDIR)/isr.o \
	obj/$(AMDCDIR)/spinlock.o \
	obj/$(AMDCDIR)/hpet.o \
	obj/$(AMDCDIR)/isr_stub.o \
	obj/$(AMDCDIR)/cpu.o \
	obj/$(AMDCDIR)/acpi.o \
	obj/$(AMDCDIR)/gdt.o \
	obj/$(AMDCDIR)/idt.o \
	obj/$(AMDCDIR)/system.o \
	obj/$(AMDCDIR)/tss.o \
	obj/$(AMDKDIR)/memcpy.o \
	obj/$(AMDKDIR)/memmove.o \
	obj/$(AMDKDIR)/memset.o \
	obj/$(AMDKDIR)/memcmp.o \
	obj/$(KERNDIR)/main.o

all: bin/wiredos

obj/kernel/%.o: $(KERNDIR)/%.s
	mkdir -p $(@D)
	$(CC) -c $< -o $@

obj/kernel/%.o: $(KERNDIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

obj/arch/amd64/cpu/%.o: $(AMDDIR)/cpu/%.s
	mkdir -p $(@D)
	$(ASM) -o $@ $<

obj/arch/amd64/cpu/%.o: $(AMDDIR)/cpu/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

obj/arch/amd64/klib/%.o: $(AMDDIR)/klib/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

obj/drivers/%.o: $(DRIVDIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

wired: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(IFLAGS)

bin/wiredos: $(OBJ)
	@echo "================"
	@echo "x64 WIRED-OS" ~
	@echo "================"
	@echo "Building FINAL ELF Executable!"
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

iso:
	@echo "Building WiredOS ISO Image..!"
	mkdir -p $(ISODIR)/boot
	@echo "Copying ELF to Boot directory.."
	cp -v bin/wiredos $(ISODIR)/boot/
	mkdir -p $(ISODIR)/boot/limine
	cp -v limine.conf thirdparty/limine/limine-bios.sys thirdparty/limine/limine-bios-cd.bin \
      thirdparty/limine/limine-uefi-cd.bin $(ISODIR)/boot/limine/
	@echo "Creating EFI Boot Tree.."
	mkdir -p $(ISODIR)/EFI/BOOT
	cp -v thirdparty/limine/BOOTX64.EFI $(ISODIR)/EFI/BOOT/
	cp -v thirdparty/limine/BOOTIA32.EFI $(ISODIR)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        $(ISODIR) -o $(ISONAME).iso
	@echo "Preparing image for Legacy BIOS.."
	thirdparty/limine/limine bios-install $(ISONAME).iso


.PHONY: clean
clean:
	@echo "Cleaning leftovers.."
	rm *.iso
	rm -rf $(ISODIR)
	rm -rf bin
	rm -rf obj
