#include "timer.h"

#include "adc.h"
#include "config.h"

data volatile uint32_t g_tick_ms = 0;

/* ------------------------------------------------------------
 * Timer0 初始化
 *  - 模式 0（16 位自动重载），1T 模式，125us 周期
 * ------------------------------------------------------------ */
void timer_init(void) {
  uint16_t reload;

  AUXR |= 0x80;
  TMOD &= 0xF0;

  reload = (uint16_t)(65536UL - (SYSCLK / 8000UL));
  TH0 = (uint8_t)(reload >> 8);
  TL0 = (uint8_t)(reload & 0xFF);

  ET0 = 1;
  TR0 = 1;

  g_tick_ms = 0;
}

/* ------------------------------------------------------------
 * 原子读取 tick（32 位跨中断访问）
 * ------------------------------------------------------------ */
uint32_t timer_get_ms(void) {
  uint32_t t;
  bit e_save = ET0;
  ET0 = 0;
  t = g_tick_ms;
  ET0 = e_save;
  return t;
}

/* ------------------------------------------------------------
 * Timer0 中断服务程序
 * ------------------------------------------------------------ */
void TIMER0_ISR(void) interrupt TMR0_VECTOR {
  static uint8_t s_tick_div = 0;

  if (++s_tick_div >= 8) {
    s_tick_div = 0;
    g_tick_ms++;
  }

  adc_start();
}