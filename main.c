#include <stdint.h>
#define MMIO32(a) (*(volatile uint32_t*)(a))

/* --- Pi Zero 2 / Pi3 peripheral map --- */
#define PERI_BASE   0x3F000000UL

/* PL011 UART0 */
#define UART_BASE   (PERI_BASE + 0x201000)
#define UART_DR     (UART_BASE + 0x00)
#define UART_FR     (UART_BASE + 0x18)
#define UART_IBRD   (UART_BASE + 0x24)
#define UART_FBRD   (UART_BASE + 0x28)
#define UART_LCRH   (UART_BASE + 0x2C)
#define UART_CR     (UART_BASE + 0x30)
#define UART_IMSC   (UART_BASE + 0x38)
#define UART_ICR    (UART_BASE + 0x44)
#define FR_TXFF     (1u<<5)
#define LCRH_FEN    (1u<<4)
#define LCRH_WLEN8  (3u<<5)
#define CR_UARTEN   (1u<<0)
#define CR_TXE      (1u<<8)
#define CR_RXE      (1u<<9)

/* GPIO（PL011用にGPIO14/15をALT0へ） */
#define GPFSEL1     (PERI_BASE + 0x200004)
#define GPPUD       (PERI_BASE + 0x200094)
#define GPPUDCLK0   (PERI_BASE + 0x200098)

/* --- BCM2836 Local (QA7) --- */
#define LOCAL_BASE               0x40000000UL
#define CORE0_TIMER_IRQCNTL     (LOCAL_BASE + 0x40)
#define CORE0_IRQ_SOURCE        (LOCAL_BASE + 0x60)
/* ビット割り当て（広く使われる定義）
   0:CNTPS, 1:CNTPNS, 2:CNTHP, 3:CNTV */
#define QA7_IRQ_CNTPS   (1u<<0)
#define QA7_IRQ_CNTPNS  (1u<<1)
#define QA7_IRQ_CNTHP   (1u<<2)
#define QA7_IRQ_CNTV    (1u<<3)

/* ---- AArch64 Generic Timer: we'll use CNTPNS (非セキュア物理) ---- */
static inline uint64_t cntfrq(void){ uint64_t v; __asm__("mrs %0,cntfrq_el0":"=r"(v)); return v; }
static inline void cntp_tval(uint32_t v){ __asm__("msr cntp_tval_el0,%0"::"r"(v)); }
static inline void cntp_enable(void){ __asm__("msr cntp_ctl_el0,%0"::"r"((uint64_t)1)); }
static inline void cntp_disable(void){ __asm__("msr cntp_ctl_el0,%0"::"r"((uint64_t)0)); }

static inline void isb(void){ __asm__ volatile("isb" ::: "memory"); }
static inline void dsb(void){ __asm__ volatile("dsb sy" ::: "memory"); }
static inline void enable_irq(void){ __asm__ volatile("msr daifclr,#2"); isb(); }
static inline void delay_cycles(unsigned n){ while(n--){ __asm__ volatile("nop"); } }

/* ---- UART ---- */
static void gpio_uart_alt0(void){
  uint32_t v = MMIO32(GPFSEL1);
  v &= ~((7u<<12)|(7u<<15));
  v |=  (4u<<12) | (4u<<15);  /* GPIO14/15 → ALT0 (TXD0/RXD0) */
  MMIO32(GPFSEL1)=v;

  MMIO32(GPPUD)=0; delay_cycles(150);
  MMIO32(GPPUDCLK0)=(1u<<14)|(1u<<15); delay_cycles(150);
  MMIO32(GPPUDCLK0)=0;
}
static void uart_init(void){
  gpio_uart_alt0();
  MMIO32(UART_CR)=0; dsb(); isb();
  MMIO32(UART_IMSC)=0; MMIO32(UART_ICR)=0x7FF;
  /* Pi実機のUARTCLKは通常48MHz想定（firmware依存）: 115200bps → 26/3 */
  MMIO32(UART_IBRD)=26; MMIO32(UART_FBRD)=3;
  MMIO32(UART_LCRH)=LCRH_WLEN8|LCRH_FEN;
  MMIO32(UART_CR)=CR_UARTEN|CR_TXE|CR_RXE; dsb(); isb();
}
static void uart_putc(char c){ while(MMIO32(UART_FR)&FR_TXFF){} MMIO32(UART_DR)=(uint32_t)c; }
static void uart_puts(const char*s){ while(*s){ if(*s=='\n') uart_putc('\r'); uart_putc(*s++);} }
static void uart_put_dec(unsigned long long x){
  char b[21]; int i=0; if(!x){uart_putc('0');return;}
  while(x && i<20){ b[i++]='0'+(x%10); x/=10; }
  while(i--) uart_putc(b[i]);
}

/* ---- timer ---- */
static volatile unsigned long long jiff=0;
static uint32_t reload=0;

void irq_handler(void){
  uint32_t src = MMIO32(CORE0_IRQ_SOURCE);
  if(src & QA7_IRQ_CNTPNS){
    cntp_tval(reload);
    if((++jiff % 1)==0) { uart_puts("[tick]\n"); }
  }
}

int main(void){
  uart_init();
  uart_puts("UART ready on Pi Zero 2.\n");

  /* 1Hzタイマ（CNTPNS） */
  uint64_t f = cntfrq();
  reload = (uint32_t)(f/1);
  cntp_disable();
  cntp_tval(reload);
  cntp_enable();

  /* CNTPNS を core0 IRQ にルーティング */
  MMIO32(CORE0_TIMER_IRQCNTL) = QA7_IRQ_CNTPNS;

  /* IRQ許可 */
  enable_irq();

  for(;;){ __asm__ volatile("wfi"); }
}

