CROSS = aarch64-elf
CC = $(CROSS)-gcc
LD = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy
CFLAGS = -O2 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -mgeneral-regs-only

all: kernel8.img

start.o: start.S
	$(CC) -c $(CFLAGS) -o $@ $<

main.o: main.c
	$(CC) -c $(CFLAGS) -o $@ $<

kernel8.elf: start.o main.o
	$(LD) -T linker.ld -o $@ $^

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.img

