CROSS = aarch64-elf
CC = $(CROSS)-gcc
LD = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy
CFLAGS = -O2 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -mgeneral-regs-only -DQEMU_VIRT

all: kernel8.img

kernel8.elf: start.o main.o
	$(LD) -T linker.ld -o $@ $^

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

run: kernel8.img
	qemu-system-aarch64 \
  -M virt,virtualization=on,gic-version=2 \
    -cpu cortex-a53 \
      -nographic -monitor none \
        -kernel kernel8.elf
          
#	qemu-system-aarch64 -M virt -cpu cortex-a53 -kernel kernel8.img -nographic

clean:
	rm -f *.o *.elf *.img

