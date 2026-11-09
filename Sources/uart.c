/* ============================================================
 * uart.c  —  纯 ISR 驱动 + 主流程接口
 * ============================================================ */

#include "uart.h"

#include "config.h"

/* ------------------------------------------------------------
 * 缓冲区配置
 * ------------------------------------------------------------ */
#if (UART_TX_BUFF_SIZE & (UART_TX_BUFF_SIZE - 1)) != 0
#error "UART_TX_BUFF_SIZE must be power of 2"
#endif

#if (UART_RX_BUFF_SIZE & (UART_RX_BUFF_SIZE - 1)) != 0
#error "UART_RX_BUFF_SIZE must be power of 2"
#endif

#define TX_MASK (UART_TX_BUFF_SIZE - 1)
#define RX_MASK (UART_RX_BUFF_SIZE - 1)

/* ------------------------------------------------------------
 * 缓冲区与索引
 *  - head/tail 必须 volatile，防止编译器优化掉跨上下文的可见性
 * ------------------------------------------------------------ */
static xdata uint8_t s_tx_buf[UART_TX_BUFF_SIZE] = {0}; /* TX 缓冲区 */
static data volatile uint8_t s_tx_head = 0;             /* 主流程写，ISR 读 */
static data volatile uint8_t s_tx_tail = 0;             /* ISR   写，主流程读 */
static data volatile uint8_t s_tx_drop = 0;             /* TX 环写满丢弃字节数 */
static data volatile uint8_t s_tx_active = 0;           /* 1 = 发送引擎已启动 */
static xdata uint8_t s_rx_buf[UART_RX_BUFF_SIZE] = {0}; /* RX 缓冲区 */
static data volatile uint8_t s_rx_head = 0;             /* ISR   写，主流程读 */
static data volatile uint8_t s_rx_tail = 0;             /* 主流程写，ISR 读 */
static data volatile uint8_t s_rx_drop = 0;             /* RX 环写满丢弃字节数 */

/* ------------------------------------------------------------
 * 内联宏：ISR 与主流程共用逻辑必须写成宏，不能写成函数
 * ------------------------------------------------------------ */
#define TX_USED() ((uint8_t)(s_tx_head - s_tx_tail) & TX_MASK)
#define TX_FREE() ((uint8_t)(TX_MASK - TX_USED()))
#define RX_USED() ((uint8_t)(s_rx_head - s_rx_tail) & RX_MASK)
#define RX_FREE() ((uint8_t)(RX_MASK - RX_USED()))

/* ============================================================
 * 初始化
 * ============================================================ */
void uart_init(void) {
  s_tx_head = 0;
  s_tx_tail = 0;
  s_tx_active = 0;
  s_tx_drop = 0;
  s_rx_head = 0;
  s_rx_tail = 0;
  s_rx_drop = 0;

  SCON = 0x50;

  AUXR &= ~0x01;
  AUXR |= 0x40;
  PCON |= 0x80;
  TMOD &= 0x0F;
  TMOD |= 0x20;
  TH1 = 0xF3;
  TL1 = 0xF3;
  TR1 = 1;

  ES = 1;
}

/* ============================================================
 * 主流程接口 —— 发送
 * 写入 TX 环；若当前空闲则启动首字节。
 * ============================================================ */
void uart_send(const uint8_t* dat, uint8_t len) {
  uint8_t i;
  uint8_t h;
  uint8_t room;

  if (len == 0) return;

  h = s_tx_head;
  room = TX_FREE();
  if (len > room) {
    s_tx_drop += len - room;
    len = room;
  }
  if (len == 0) return;

  for (i = 0; i < len; i++) {
    s_tx_buf[h] = dat[i];
    h = (uint8_t)((h + 1) & TX_MASK);
  }
  s_tx_head = h;

  if (!s_tx_active) {
    TI = 1;
  }
}

/* printf 重定向 */
char putchar(char c) {
  uart_send(&c, 1);
  return c;
}

/* ============================================================
 * 主流程接口 —— 接收
 * ============================================================ */
uint8_t uart_read_byte(uint8_t* b) {
  if (s_rx_tail == s_rx_head) return 0;
  *b = s_rx_buf[s_rx_tail];
  s_rx_tail = (uint8_t)((s_rx_tail + 1) & RX_MASK);
  return 1;
}

/* ============================================================
 * 主流程接口 —— 状态查询
 * ============================================================ */
uint8_t uart_tx_used(void) { return TX_USED(); }
uint8_t uart_rx_used(void) { return RX_USED(); }
uint8_t uart_tx_drop(void) { return s_tx_drop; }
uint8_t uart_rx_drop(void) { return s_rx_drop; }

/* ============================================================
 * UART1 中断服务
 * ============================================================ */
void UART1_ISR(void) interrupt UART1_VECTOR {
  /* ---------- TX：上一字节发完，接力下一字节 ---------- */
  if (TI) {
    TI = 0;
    if (s_tx_tail != s_tx_head) {
      SBUF = s_tx_buf[s_tx_tail];
      s_tx_tail = (uint8_t)((s_tx_tail + 1) & TX_MASK);
      s_tx_active = 1;
    } else {
      s_tx_active = 0;
    }
  }

  /* ---------- RX：收到一字节，立即入队 ---------- */
  if (RI) {
    RI = 0;
    {
      uint8_t b = SBUF;
      uint8_t next = (uint8_t)((s_rx_head + 1) & RX_MASK);
      if (next != s_rx_tail) {
        s_rx_buf[s_rx_head] = b;
        s_rx_head = next;
      } else {
        s_rx_drop++;
      }
    }
  }
}
