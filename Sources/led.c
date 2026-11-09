#include "led.h"

sbit LED_BIT = P3 ^ 2;

#define LED_ON() (LED_BIT = 0)
#define LED_OFF() (LED_BIT = 1)

static xdata led_mode_t s_mode = LED_MODE_OFF;
static xdata uint32_t s_last_change = 0;

void led_init(void) {
  s_mode = LED_MODE_OFF;
  s_last_change = 0;

  P3M0 |= 0x04;
  P3M1 &= (uint8_t)~0x04;
  LED_OFF();
}

void led_set_mode(led_mode_t mode) {
  if (s_mode != mode) {
    s_mode = mode;
  }
}

void led_poll(uint32_t now) {
  uint32_t elapsed;
  uint16_t period;
  uint16_t on_time;

  if (s_mode == LED_MODE_OFF) {
    LED_OFF();
    return;
  }
  if (s_mode == LED_MODE_ON) {
    LED_ON();
    return;
  }

  switch (s_mode) {
    case LED_MODE_FLASH_SLOW:
      period = 1000;
      on_time = 100;
      break;
    case LED_MODE_FLASH_FAST:
      period = 200;
      on_time = 100;
      break;
    case LED_MODE_FLASH_URGENT:
      period = 100;
      on_time = 50;
      break;
    default:
      LED_OFF();
      return;
  }

  elapsed = now - s_last_change;
  if (elapsed >= period) {
    s_last_change = now;
    elapsed = 0;
  }

  if (elapsed < on_time) {
    LED_ON();
  } else {
    LED_OFF();
  }
}