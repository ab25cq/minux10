#include <stdint.h>
#define MMIO32(a) (*(volatile uint32_t*)(a))

/* ---- QEMU -M virt (GICv2 / PL011 / CNTV) ---- */
#define UART_BASE   0x09000000UL          /* PL011 */
#define GICD_BASE   0x08000000UL          /* Distributor */
#define GICC_BASE   0x08010000UL          /* CPU IF */
#define TIMER_PPI_ID 27u                  /* CNTV IRQ (PPI=27) */

/* UART regs */
#define UART_DR    (UART_BASE + 0x00)
#define UART_FR    (UART_BASE + 0x18)
#define UART_LCRH  (UART_BASE + 0x2C)
#define UART_CR    (UART_BASE + 0x30)
#define UART_IMSC  (UART_BASE + 0x38)
#define UART_ICR   (UART_BASE + 0x44)
#define FR_TXFF    (1u<<5)
#define LCRH_FEN   (1u<<4)
#define LCRH_WLEN8 (3u<<5)
#define CR_UARTEN  (1u<<0)
#define CR_TXE     (1u<<8)
#define CR_RXE     (1u<<9)

static inline void isb(void){ __asm__ volatile("isb" ::: "memory"); }
static inline void dsb(void){ __asm__ volatile("dsb sy" ::: "memory"); }

static void uart_init(void){
  MMIO32(UART_CR)=0;
  MMIO32(UART_IMSC)=0;
  MMIO32(UART_ICR)=0x7FF;
  MMIO32(UART_LCRH)=LCRH_WLEN8|LCRH_FEN;
  MMIO32(UART_CR)=CR_UARTEN|CR_TXE|CR_RXE;
}
static void uart_putc(char c){ while(MMIO32(UART_FR)&FR_TXFF){} MMIO32(UART_DR)=(uint32_t)c; }
static void uart_puts(const char*s){ while(*s){ if(*s=='\n') uart_putc('\r'); uart_putc(*s++);} }

/* ---- GICv2 ---- */
#define GICD_CTLR         (GICD_BASE + 0x000)
#define GICD_ISENABLER(n) (GICD_BASE + 0x100 + 4*(n))
#define GICD_IPRIORITYR(n)(GICD_BASE + 0x400 + 4*(n))
#define GICC_CTLR         (GICC_BASE + 0x000)
#define GICC_PMR          (GICC_BASE + 0x004)
#define GICC_IAR          (GICC_BASE + 0x00C)
#define GICC_EOIR         (GICC_BASE + 0x010)

static void gic_init(void){
  /* Distributor ON */
  MMIO32(GICD_CTLR)=1;                 /* Group0 enable */
  /* Enable PPI 30 */
  MMIO32(GICD_ISENABLER(0)) = (1u<<TIMER_PPI_ID);
  /* Priority */
  uint32_t idx=TIMER_PPI_ID/4, sh=(TIMER_PPI_ID%4)*8;
  uint32_t pri=MMIO32(GICD_IPRIORITYR(idx));
  pri&=~(0xFFu<<sh); pri|=(0x80u<<sh);
  MMIO32(GICD_IPRIORITYR(idx))=pri;
  /* CPU IF */
  MMIO32(GICC_PMR)=0xFF;
  MMIO32(GICC_CTLR)=1;                 /* Group0 enable */
}

/* ---- Generic Timer (CNTV) ---- */
static inline uint64_t cntfrq(void){ uint64_t x; __asm__("mrs %0,cntfrq_el0":"=r"(x)); return x; }
static inline void cntv_tval(uint32_t v){ __asm__("msr cntv_tval_el0,%0"::"r"(v)); }
static inline void cntv_enable(void){ __asm__("msr cntv_ctl_el0,%0"::"r"((uint64_t)1)); }
static inline void cntv_disable(void){ __asm__("msr cntv_ctl_el0,%0"::"r"((uint64_t)0)); }

static volatile uint64_t jiff=0;
static uint32_t reload=0;

static inline void enable_irq(void){ __asm__ volatile("msr daifclr,#2"); isb(); }

void irq_handler(void){
  uint32_t iar=MMIO32(GICC_IAR);
  uint32_t id = iar & 0x3FFu;
  if(id==TIMER_PPI_ID){
    cntv_tval(reload);
    if((++jiff % 1)==0){
      uart_puts("[tick]\n");
    }
  }
  MMIO32(GICC_EOIR)=iar;
}

int main(void){
  uart_init();
  uart_puts("UART ready on virt.\n");

  gic_init();
  uart_puts("GICv2 ready.\n");

  uint64_t f=cntfrq();       /* e.g. 50–62.5MHz */
  reload=(uint32_t)(f/1);    /* 1Hz */
  cntv_disable();
  cntv_tval(reload);
  cntv_enable();

  uart_puts("CNTV 1Hz started.\n");

  enable_irq();              /* EL1のIビット解除 */
  for(;;){ __asm__ volatile("wfi"); }
}

