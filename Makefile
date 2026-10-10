CC      = gcc
AS      = nasm
LD      = ld
CFLAGS  = -m32 -ffreestanding -fno-pic -fno-stack-protector -fno-builtin \
          -nostdlib -O2 -Wall -Wextra -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib -z noexecstack

C_SRC   = $(wildcard arch/*.c drivers/*.c kernel/*.c lib/*.c)
ASM_SRC = $(wildcard boot/*.asm arch/*.asm)
OBJ     = $(patsubst %.c,build/%.o,$(C_SRC)) $(patsubst %.asm,build/%.o,$(ASM_SRC))

build/myos.elf: $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

build/%.o: %.c include/kernel.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

run: build/myos.elf
	qemu-system-i386 -kernel $< -serial stdio

run-headless: build/myos.elf
	qemu-system-i386 -kernel $< -serial stdio -display none

test: build/myos.elf
	./tests/run_tests.sh

clean:
	rm -rf build

.PHONY: run run-headless test clean
