#include "app.h"

#include "led.h"
#include "timer.h"

/* ============================================================
 * 内部状态
 * ============================================================ */
static xdata app_state_t s_state = APP_ST_IDLE;
static xdata uint32_t s_state_tick = 0;
static xdata uint16_t s_state_timeout = 0;
static xdata app_shutdown_fn s_shutdown_hook = NULL;

#define APP_EVENT_QUEUE_SIZE 8
#define APP_EVENT_QUEUE_MASK (APP_EVENT_QUEUE_SIZE - 1)

#if (APP_EVENT_QUEUE_SIZE & APP_EVENT_QUEUE_MASK) != 0
#error "APP_EVENT_QUEUE_SIZE must be a power of 2"
#endif

static data uint8_t s_evt_queue[APP_EVENT_QUEUE_SIZE];
static data uint8_t s_evt_head = 0;
static data uint8_t s_evt_count = 0;

/* ============================================================
 * 初始化
 * ============================================================ */
void app_init(void) {
  s_state = APP_ST_IDLE;
  s_state_tick = 0;
  s_state_timeout = 0;
  s_shutdown_hook = NULL;
  s_evt_head = 0;
  s_evt_count = 0;
  led_set_mode(LED_MODE_FLASH_SLOW);
}

void app_set_shutdown_hook(app_shutdown_fn fn) {
  if (fn) s_shutdown_hook = fn;
}

/* ============================================================
 * 事件投递
 * ============================================================ */
static void app_queue_remove(uint8_t offset) {
  uint8_t i;

  for (i = offset; i + 1 < s_evt_count; i++) {
    s_evt_queue[(s_evt_head + i) & APP_EVENT_QUEUE_MASK] =
        s_evt_queue[(s_evt_head + i + 1) & APP_EVENT_QUEUE_MASK];
  }
  s_evt_count--;
}

uint8_t app_post_event(app_event_t evt) {
  uint8_t i;
  uint8_t removed;

  if (evt <= APP_EVT_NONE || evt >= APP_EVT_TIMEOUT) return 0;

  if (s_evt_count >= APP_EVENT_QUEUE_SIZE) {
    if (evt != APP_EVT_RESTART) return 0;

    /* 队列满时优先保留重启事件，丢弃最早的非重启事件腾出空间。 */
    removed = 0;
    for (i = 0; i < s_evt_count; i++) {
      if (s_evt_queue[(s_evt_head + i) & APP_EVENT_QUEUE_MASK] != APP_EVT_RESTART) {
        app_queue_remove(i);
        removed = 1;
        break;
      }
    }

    if (!removed) return 1;
  }

  s_evt_queue[(s_evt_head + s_evt_count) & APP_EVENT_QUEUE_MASK] = (uint8_t)evt;
  s_evt_count++;
  return 1;
}

/* ============================================================
 * 状态迁移
 * ============================================================ */
static void app_transition(app_state_t next, uint32_t now) {
  if (s_state != next) {
    s_state = next;
    s_state_tick = now;
    s_state_timeout = 0;

    switch (s_state) {
      case APP_ST_IDLE:
        led_set_mode(LED_MODE_FLASH_SLOW);
        break;
      case APP_ST_DEBUG:
        led_set_mode(LED_MODE_FLASH_FAST);
        break;
      case APP_ST_RST_PEND:
        led_set_mode(LED_MODE_FLASH_URGENT);
        break;
      default:
        led_set_mode(LED_MODE_OFF);
        break;
    }
  }
}

/* ============================================================
 * 事件处理
 * ============================================================ */
static void app_handle(app_event_t evt, uint32_t now) {
  switch (s_state) {
    case APP_ST_IDLE:
      if (evt == APP_EVT_DEBUG_TOGGLE || evt == APP_EVT_DEBUG_ENTER) {
        app_transition(APP_ST_DEBUG, now);
      } else if (evt == APP_EVT_RESTART) {
        app_transition(APP_ST_RST_PEND, now);
        s_state_timeout = 2000;
      }
      break;

    case APP_ST_DEBUG:
      if (evt == APP_EVT_DEBUG_TOGGLE || evt == APP_EVT_DEBUG_EXIT) {
        app_transition(APP_ST_IDLE, now);
      } else if (evt == APP_EVT_RESTART) {
        app_transition(APP_ST_RST_PEND, now);
        s_state_timeout = 2000;
      }
      break;

    case APP_ST_RST_PEND:
      if (evt == APP_EVT_TIMEOUT) {
        if (s_shutdown_hook) s_shutdown_hook();
        EA = 0;
        IAP_CONTR = 0x20;
        while (1);
      }
      break;

    default:
      break;
  }
}

/* ============================================================
 * 主轮询：超时检查 + 事件分发
 * ============================================================ */
void app_poll(uint32_t now) {
  app_event_t evt;

  /* 超时检查 */
  if (s_state_timeout != 0) {
    if ((now - s_state_tick) >= s_state_timeout) {
      s_state_timeout = 0;
      app_handle(APP_EVT_TIMEOUT, now);
    }
  }

  while (s_evt_count != 0) {
    evt = (app_event_t)s_evt_queue[s_evt_head];
    s_evt_head = (uint8_t)((s_evt_head + 1) & APP_EVENT_QUEUE_MASK);
    s_evt_count--;
    app_handle(evt, now);
  }
}

/* ============================================================
 * 查询接口
 * ============================================================ */
app_state_t app_get_state(void) { return s_state; }

const char* app_state_name(app_state_t st) {
  switch (st) {
    case APP_ST_IDLE:
      return "ST_IDLE";
    case APP_ST_DEBUG:
      return "ST_DEBUG";
    case APP_ST_RST_PEND:
      return "ST_RST_PEND";
    default:
      return "ST_UNKNOWN";
  }
}