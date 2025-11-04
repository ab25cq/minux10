#include <stdint.h>

#define MMIO32(addr) (*(volatile uint32_t *)(addr))

#define PERIPHERAL_BASE 0x3F000000UL

/* PL011 UART0 */
#define UART0_BASE   (PERIPHERAL_BASE + 0x201000)
#define UART0_DR     (UART0_BASE + 0x00)
#define UART0_FR     (UART0_BASE + 0x18)
#define UART0_IBRD   (UART0_BASE + 0x24)
#define UART0_FBRD   (UART0_BASE + 0x28)
#define UART0_LCRH   (UART0_BASE + 0x2C)
#define UART0_CR     (UART0_BASE + 0x30)
#define UART0_IMSC   (UART0_BASE + 0x38)
#define UART0_ICR    (UART0_BASE + 0x44)

/* GPIO for UART (GPIO14/15) */
#define GPFSEL1      (PERIPHERAL_BASE + 0x200004)
#define GPPUD        (PERIPHERAL_BASE + 0x200094)
#define GPPUDCLK0    (PERIPHERAL_BASE + 0x200098)

/* Core local interrupt controller (QA7) */
#define LOCAL_BASE             0x40000000UL
#define CORE0_TIMER_IRQCNTL    (LOCAL_BASE + 0x40)
#define CORE0_IRQ_SOURCE       (LOCAL_BASE + 0x60)

#define CORE_IRQ_SRC_CNTV      (1u << 1)  /* bit1 = CNTV IRQ */

/* UART bits/flags */
#define FR_TXFF      (1u << 5)
#define LCRH_FEN     (1u << 4)
#define LCRH_WLEN8   (3u << 5)
#define CR_UARTEN    (1u << 0)
#define CR_TXE       (1u << 8)
#define CR_RXE       (1u << 9)

static volatile uint64_t g_tick_count = 0;
static uint32_t g_timer_interval = 0;

static inline void dsb(void){ __asm__ volatile("dsb sy" ::: "memory"); }
static inline void isb(void){ __asm__ volatile("isb" ::: "memory"); }

static inline void delay_cycles(unsigned count)
{
    while (count--) {
        __asm__ volatile("nop");
    }
}

static inline void disable_irq(void)
{
    __asm__ volatile("msr daifset, #2" ::: "memory");
    isb();
}

static inline void enable_irq(void)
{
    __asm__ volatile("msr daifclr, #2" ::: "memory");
    isb();
}

static inline uint64_t cnt_read_freq(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static inline void cntv_write_tval(uint32_t value)
{
    uint64_t tmp = value;
    __asm__ volatile("msr cntv_tval_el0, %0" :: "r"(tmp));
}

static inline void cntv_disable(void)
{
    uint64_t zero = 0;
    __asm__ volatile("msr cntv_ctl_el0, %0" :: "r"(zero));
}

static inline void cntv_enable(void)
{
    uint64_t one = 1;
    __asm__ volatile("msr cntv_ctl_el0, %0" :: "r"(one));
}

static void gpio_setup_uart_pins(void)
{
    uint32_t sel1 = MMIO32(GPFSEL1);
    sel1 &= ~((7u << 12) | (7u << 15));
    sel1 |=  (4u << 12) | (4u << 15);
    MMIO32(GPFSEL1) = sel1;

    MMIO32(GPPUD) = 0;
    delay_cycles(150);
    MMIO32(GPPUDCLK0) = (1u << 14) | (1u << 15);
    delay_cycles(150);
    MMIO32(GPPUDCLK0) = 0;
}

static void uart_init(void)
{
    gpio_setup_uart_pins();

    MMIO32(UART0_CR) = 0;
    dsb(); isb();

    MMIO32(UART0_IMSC) = 0;
    MMIO32(UART0_ICR)  = 0x7FF;

    MMIO32(UART0_IBRD) = 26;   /* UARTCLK=48MHz -> 115200 bps */
    MMIO32(UART0_FBRD) = 3;
    MMIO32(UART0_LCRH) = LCRH_WLEN8 | LCRH_FEN;

    MMIO32(UART0_CR) = CR_UARTEN | CR_TXE | CR_RXE;
    dsb(); isb();
}

static void uart_putc(char c)
{
    while (MMIO32(UART0_FR) & FR_TXFF) { /* wait */ }
    MMIO32(UART0_DR) = (uint32_t)(c & 0xFF);
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

static void uart_put_dec(uint64_t value)
{
    char buf[21];
    unsigned idx = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value && idx < sizeof(buf)) {
        buf[idx++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (idx) {
        uart_putc(buf[--idx]);
    }
}

static void timer_init(uint32_t hz)
{
    uint64_t freq = cnt_read_freq();
    if (hz == 0) {
        hz = 1;
    }

    g_timer_interval = (uint32_t)(freq / hz);
    if (g_timer_interval == 0) {
        g_timer_interval = 1;
    }

    cntv_disable();
    dsb(); isb();

    cntv_write_tval(g_timer_interval);
    cntv_enable();

    MMIO32(CORE0_TIMER_IRQCNTL) = CORE_IRQ_SRC_CNTV; /* route CNTV to core0 IRQ */
    isb();
}

void irq_handler(void)
{
    uint32_t source = MMIO32(CORE0_IRQ_SOURCE);

    if (source & CORE_IRQ_SRC_CNTV) {
        cntv_write_tval(g_timer_interval);
        g_tick_count++;
        uart_puts("tick ");
        uart_put_dec(g_tick_count);
        uart_putc('\n');
    }
}

int main(void)
{
    disable_irq();

    uart_init();
    uart_puts("PL011 ready on Raspberry Pi Zero 2 / QEMU raspi3b.\n");

    timer_init(1);  /* 1Hz tick */
    uart_puts("Virtual timer IRQ enabled.\n");

    enable_irq();

    for (;;) {
        __asm__ volatile("wfi");
    }
}

