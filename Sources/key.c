#include "key.h"

/* 系统按键：P3.3，低电平有效 */
sbit KEY_BIT = P3 ^ 3;

/* ============================================================
 * 内部状态
 * ============================================================ */

typedef enum {
  STATE_IDLE = 0,
  STATE_DEBOUNCE,
  STATE_PRESSED,
  STATE_WAIT_RELEASE,
#if KEY_DOUBLE_CLICK_ENABLE
  STATE_WAIT_DOUBLE,
  STATE_DOUBLE_DEBOUNCE,
#endif
} key_state_t;

typedef struct {
  key_state_t state;
  uint8_t last_pressed;
  uint32_t press_time;
  uint32_t release_time;
  key_event_t event;
} key_ctx_t;

static xdata key_ctx_t s_key;

/* ============================================================
 * 接口实现
 * ============================================================ */

void key_init(void) {
  s_key.state = STATE_IDLE;
  s_key.last_pressed = 0;
  s_key.press_time = 0;
  s_key.release_time = 0;
  s_key.event = KEY_EVENT_NONE;

  P3M0 &= (uint8_t)~0x08;
  P3M1 &= (uint8_t)~0x08;
  KEY_BIT = 1;
}

void key_poll(uint32_t now) {
  uint8_t pressed = KEY_BIT ? 0 : 1;

  switch (s_key.state) {
    case STATE_IDLE:
      if (pressed && !s_key.last_pressed) {
        s_key.state = STATE_DEBOUNCE;
        s_key.press_time = now;
      }
      break;

    case STATE_DEBOUNCE:
      if (pressed) {
        if ((now - s_key.press_time) >= KEY_DEBOUNCE_MS) {
          s_key.state = STATE_PRESSED;
          s_key.event = KEY_EVENT_PRESS;
        }
      } else {
        s_key.state = STATE_IDLE;
      }
      break;

    case STATE_PRESSED:
      if (pressed) {
        if ((now - s_key.press_time) >= KEY_LONG_PRESS_MS) {
          s_key.event = KEY_EVENT_LONG_PRESS;
          s_key.state = STATE_WAIT_RELEASE;
        }
      } else {
        s_key.release_time = now;
#if KEY_DOUBLE_CLICK_ENABLE
        s_key.state = STATE_WAIT_DOUBLE;
#else
        s_key.event = KEY_EVENT_SHORT_CLICK;
        s_key.state = STATE_IDLE;
#endif
      }
      break;

#if KEY_DOUBLE_CLICK_ENABLE
    case STATE_WAIT_DOUBLE:
      if (pressed) {
        s_key.state = STATE_DOUBLE_DEBOUNCE;
        s_key.press_time = now;
      } else if ((now - s_key.release_time) >= KEY_DOUBLE_CLICK_MS) {
        s_key.event = KEY_EVENT_SHORT_CLICK;
        s_key.state = STATE_IDLE;
      }
      break;

    case STATE_DOUBLE_DEBOUNCE:
      if (pressed) {
        if ((now - s_key.press_time) >= KEY_DEBOUNCE_MS) {
          s_key.event = KEY_EVENT_DOUBLE_CLICK;
          s_key.state = STATE_WAIT_RELEASE;
        }
      } else {
        s_key.state = STATE_WAIT_DOUBLE;
      }
      break;
#endif

    case STATE_WAIT_RELEASE:
      if (!pressed) {
        s_key.state = STATE_IDLE;
      }
      break;

    default:
      s_key.state = STATE_IDLE;
      break;
  }

  s_key.last_pressed = pressed;
}

key_event_t key_get_event(void) {
  key_event_t evt = s_key.event;
  s_key.event = KEY_EVENT_NONE;
  return evt;
}