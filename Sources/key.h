#ifndef __KEY_H__
#define __KEY_H__

#include "config.h"

/* ============================================================
 * 系统按键模块（单例）
 *
 * 硬件：
 *   - 引脚：P3.3
 *   - 模式：准双向口 + 内部弱上拉
 *   - 极性：低电平有效
 *
 * 事件：
 *   KEY_EVENT_PRESS        按下瞬间（去抖后）
 *   KEY_EVENT_SHORT_CLICK  短按单击
 *   KEY_EVENT_LONG_PRESS   长按触发
 *   KEY_EVENT_DOUBLE_CLICK 双击触发（需开启 KEY_DOUBLE_CLICK_ENABLE）
 *
 * 说明：
 *   - 引脚与有效电平在本模块内部定义，对外不暴露
 *   - 接口无参，全工程仅支持一个系统按键
 *   - key_poll 需周期调用，建议 5~10ms 一次
 *   - 事件在产生时置位，由 key_get_event 读取后清除
 * ============================================================ */

typedef enum {
  KEY_EVENT_NONE = 0,
  KEY_EVENT_PRESS,
  KEY_EVENT_SHORT_CLICK,
  KEY_EVENT_LONG_PRESS,
  KEY_EVENT_DOUBLE_CLICK,
} key_event_t;

/* ============================================================
 * 功能开关
 * ============================================================ */

/* 双击支持：1=开启（默认），0=关闭
 *   - 开启：短按上报需等待 KEY_DOUBLE_CLICK_MS 窗口超时
 *   - 关闭：短按释放后立即上报，代码路径简化
 */
#ifndef KEY_DOUBLE_CLICK_ENABLE
#define KEY_DOUBLE_CLICK_ENABLE 1
#endif

/* ============================================================
 * 时间参数
 * ============================================================ */

#define KEY_DEBOUNCE_MS 20
#define KEY_LONG_PRESS_MS 3000
#define KEY_DOUBLE_CLICK_MS 300

/* ============================================================
 * 接口
 * ============================================================ */

/* 初始化：配置引脚，清零内部状态 */
void key_init(void);

/* 扫描：非阻塞，需周期调用 */
void key_poll(uint32_t now);

/* 获取并清除当前事件 */
key_event_t key_get_event(void);

#endif /* __KEY_H__ */