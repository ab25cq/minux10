volatile unsigned int* const UART0_DR = (unsigned int*)0x3F201000;

static void uart_putc(char c) {
    *UART0_DR = (unsigned int)c;
}

static void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

int main(void) {
    uart_puts("Hello from Pi Zero 2 on QEMU!\r\n");
    while (1);
    return 0;
}

