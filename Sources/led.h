#ifndef __LED_H__
#define __LED_H__

#include "config.h"

/* ============================================================
 * 系统 LED 指示模块
 *
 * 硬件：
 *   - 引脚：P3.2
 *   - 模式：推挽输出
 *   - 极性：低电平点亮
 *
 * 模式：
 *   LED_MODE_OFF          关闭
 *   LED_MODE_ON           常亮
 *   LED_MODE_FLASH_SLOW   慢闪（周期 1000ms，亮 100ms）
 *   LED_MODE_FLASH_FAST   快闪（周期 200ms，亮 100ms）
 *   LED_MODE_FLASH_URGENT 急闪（周期 100ms，亮 50ms）
 *
 * 说明：
 *   - 模式名称仅描述闪烁行为，与状态机语义无关
 *   - 由应用层根据状态选择合适的模式
 * ============================================================ */

typedef enum {
  LED_MODE_OFF = 0,
  LED_MODE_ON,
  LED_MODE_FLASH_SLOW,
  LED_MODE_FLASH_FAST,
  LED_MODE_FLASH_URGENT
} led_mode_t;

/* 初始化：配置引脚为推挽输出，默认关闭 */
void led_init(void);

/* 设置模式：模式变化时立即从新周期起点开始 */
void led_set_mode(led_mode_t mode);

/* 轮询：非阻塞，建议每 1ms 调用一次 */
void led_poll(uint32_t now);

#endif /* __LED_H__ */