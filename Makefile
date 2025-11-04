CROSS = aarch64-elf
CC = $(CROSS)-gcc
LD = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy
CFLAGS = -O2 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -mgeneral-regs-only

all: kernel8.elf

start.o: start.S
	$(CC) -c $(CFLAGS) -o $@ $<

main.o: main.c
	$(CC) -c $(CFLAGS) -o $@ $<

kernel8.elf: start.o main.o
	$(LD) -T linker.ld -o $@ $^

run: kernel8.elf
	qemu-system-aarch64 \
      -M virt,virtualization=on,gic-version=2 \
      -cpu cortex-a53 \
      -nographic -monitor none \
      -kernel kernel8.elf

clean:
	rm -f *.o *.elf *.img

