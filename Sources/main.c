#include <STC8G.H>

#include "adc.h"
#include "app.h"
#include "config.h"
#include "key.h"
#include "led.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

/* ------------------------------------------------------------
 * 系统初始化
 * ------------------------------------------------------------ */
static void sys_init(void) {
  P_SW2 |= 0x80;

  P0M0 = 0x00;
  P0M1 = 0x00;
  P1M0 = 0x00;
  P1M1 = 0x00;
  P2M0 = 0x00;
  P2M1 = 0x00;
  P3M0 = 0x00;
  P3M1 = 0x00;
  P4M0 = 0x00;
  P4M1 = 0x00;
  P5M0 = 0x00;
  P5M1 = 0x00;
}

void delay_ms(uint16_t ms) {
  uint16_t i;
  do {
    i = MAIN_Fosc / 10456;
    while (--i);
  } while (--ms);
}

static void app_shutdown(void) {
  /* ---- 关闭关键外设 ---- */
  P5 = 0xff;
}

/* ------------------------------------------------------------
 * 主函数
 * ------------------------------------------------------------ */
void main(void) {
  uint32_t now = 0;
  uint8_t now8 = 0;
  uint8_t last_1ms = 0;
  uint8_t last_10ms = 0;
  uint8_t last_100ms = 0;

  IE = 0x00;
  delay_ms(10);
  sys_init();
  timer_init();
  uart_init();
  adc_init();
  key_init();
  led_init();
  app_init();
  app_set_shutdown_hook(app_shutdown);
  EA = 1;

  printf("system ready\r\n");
  shell_init();

  WDT_CONTR = 0xA3;  // 启动看门狗定时，约262ms@24Mhz

  while (1) {  // 当前主循环一次约16us
    now = timer_get_ms();
    now8 = (uint8_t)now;

    /* ---- 1ms 任务 ---- */
    if ((uint8_t)(now8 - last_1ms) >= 1) {
      last_1ms = now8;
      led_poll(now);
      app_poll(now);
      shell_poll(now);
    }

    /* ---- 10ms 任务 ---- */
    if ((uint8_t)(now8 - last_10ms) >= 10) {
      last_10ms = now8;
      key_poll(now);

      { /* ---- 系统按键事件投递 ---- */
        key_event_t evt = key_get_event();
        if (evt == KEY_EVENT_SHORT_CLICK) {
          printf("short\r\n");
        } else if (evt == KEY_EVENT_LONG_PRESS) {
          printf("long\r\n");
          if (!app_post_event(APP_EVT_RESTART)) printf("failed to queue restart event\r\n");
        } else if (evt == KEY_EVENT_DOUBLE_CLICK) {
          printf("double\r\n");
          if (!app_post_event(APP_EVT_DEBUG_TOGGLE)) printf("event queue full\r\n");
        }
      }
    }

    /* ---- 100ms 任务 ---- */
    if ((uint8_t)(now8 - last_100ms) >= 100) {
      last_100ms = now8;
      WDT_CONTR = 0xB3;
    }
  }
}