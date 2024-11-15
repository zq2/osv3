# Compiler settings
CC = gcc
AS = nasm
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -I./kernel/include
LDFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
ASFLAGS = -f elf32

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
KERNEL_SRC = $(KERNEL_DIR)/src
KERNEL_INCLUDE = $(KERNEL_DIR)/include

# Source files
KERNEL_SRCS = $(wildcard $(KERNEL_SRC)/*.c)
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
ASM_SRCS = $(wildcard $(BOOT_DIR)/*.asm)
ASM_OBJS = $(ASM_SRCS:.asm=.o)

# Output files
KERNEL = myos.bin
ISO = myos.iso

.PHONY: all clean run

all: $(ISO)

$(KERNEL): $(ASM_OBJS) $(KERNEL_OBJS)
	$(CC) -T linker.ld -o $@ $(LDFLAGS) $^

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

$(ISO): $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $@ isodir

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f $(KERNEL_OBJS) $(ASM_OBJS) $(KERNEL) $(ISO)
	rm -rf isodir