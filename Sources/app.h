#ifndef __APP_H__
#define __APP_H__

#include "config.h"

/* ============================================================
 * 应用层：状态机 + 系统行为协调
 * ============================================================ */

typedef enum {
  APP_ST_IDLE = 0,
  APP_ST_DEBUG,
  APP_ST_RST_PEND,
  APP_ST_MAX
} app_state_t;

typedef enum {
  APP_EVT_NONE = 0,
  APP_EVT_DEBUG_TOGGLE,   /* 短按切换调试模式 */
  APP_EVT_DEBUG_ENTER,    /* Shell 命令进入调试 */
  APP_EVT_DEBUG_EXIT,     /* Shell 命令退出调试 */
  APP_EVT_RESTART,        /* 长按或 Shell 命令重启 */
  APP_EVT_TIMEOUT,        /* 内部超时 */
  APP_EVT_MAX
} app_event_t;

typedef void (*app_shutdown_fn)(void);

void app_init(void);
void app_poll(uint32_t now);
/* 仅主循环调用；非法事件或普通事件队列满时返回 0；重启事件优先保留。 */
uint8_t app_post_event(app_event_t evt);
void app_set_shutdown_hook(app_shutdown_fn fn);

app_state_t app_get_state(void);
const char* app_state_name(app_state_t st);

#endif /* __APP_H__ */