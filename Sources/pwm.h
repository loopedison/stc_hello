#ifndef __PWM_H__
#define __PWM_H__

#include "config.h"

/* ============================================================
 * PWM 模块接口（PCA 8 位 PWM 模式）
 *
 * 功能：
 *   - pwm_init()     : 初始化 PCA 为 8 位 PWM，输出引脚为 P3.2
 *   - pwm_set_duty() : 设置占空比
 *   - pwm_get_duty() : 读取占空比
 *
 * 说明：
 *   - 占空比范围 0~255，对应 0%~100%
 * ============================================================ */

void pwm_init(void);
void pwm_set_duty(uint8_t duty);
uint8_t pwm_get_duty(void);

#endif /* __PWM_H__ */