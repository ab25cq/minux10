CROSS = aarch64-linux-gnu
CC = $(CROSS)-gcc
LD = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy
CFLAGS = -O2 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -mgeneral-regs-only

all: kernel8.img

kernel8.elf: start.o main.o
	$(LD) -T linker.ld -o $@ $^

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

run: kernel8.img
	qemu-system-aarch64 -M raspi3b -cpu cortex-a53 -kernel kernel8.img -serial stdio -display none

clean:
	rm -f *.o *.elf *.img

