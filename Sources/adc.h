
#ifndef __ADC_H__
#define __ADC_H__

#include "config.h"

/* ============================================================
 * ADC 模块接口
 *
 * 数据流：
 *   Timer0 ISR (125us) → adc_start() → ADC 转换完成
 *                                          → ADC ISR
 *                                               ├─ P5.4/P5.5 交替采样
 *                                               ├─ 每通道累加 4 次求均值
 *                                               └─ 写入对应环形缓冲区
 *   主上下文 → 按通道读取最近 len 个均值样本
 *
 * 设计：
 *   - 每通道缓冲区永远保留最近 ADC_BUFF_SIZE 个均值样本（覆盖旧数据）
 *   - ISR 只写 head ，主循环只读；拷贝快照时关 EADC
 * ============================================================ */

/* ---- 生命周期 ---- */
void adc_init(void);

/* ---- 启动接口：由 Timer0 ISR 调用 ---- */
void adc_start(void);

typedef enum {
  ADC_CHANNEL_P54 = 0,
  ADC_CHANNEL_P55,
  ADC_CHANNEL_COUNT
} adc_channel_t;

/* ---- 消费接口（主上下文）----
 * adc_read: 拷贝指定通道最近 len 个均值样本到 buf
 *           - len 不能超过 ADC_BUFF_SIZE
 *           - 拷贝期间关 EADC，避免撕裂
 */
void adc_read(adc_channel_t channel, uint16_t* buf, uint8_t len);

/* 指定通道最近一次均值（10 位，0..1023） */
uint16_t adc_last_value(adc_channel_t channel);

#endif /* __ADC_H__ */