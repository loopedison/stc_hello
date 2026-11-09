#include "adc.h"

#include "config.h"

#if ADC_BUFF_SIZE & (ADC_BUFF_SIZE - 1) != 0
#error "ADC_BUFF_SIZE must be power of 2"
#endif

/* ============================================================
 * ADC 硬件参数
 *  - 通道：P5.4 = ADC4，P5.5 = ADC5，交替采样
 *  - 精度：10 位，右对齐
 *  - 启动：Timer0 ISR 触发一次
 *  - 输出：每通道每 4 次转换取一次均值 → 每通道 1kHz 有效样本率
 * ============================================================ */

/* ADC 通道号 */
#define ADC_CH_P54 4
#define ADC_CH_P55 5

/* ADC_CONTR 位定义 */
#define ADCCONTR_POWER 0x80
#define ADCCONTR_START 0x40
#define ADCCONTR_FLAG 0x20
#define ADCCONTR_CH_MASK 0x0F

/* 每通道均值窗口：4 次；两通道交替采样时每路输出 1kHz */
#define ADC_AVG_WIN 4

/* START 卡死守护：连续 2 次 START 未归零则复位 ADC */
#define ADC_START_STUCK_LIMIT 2

/* ============================================================
 * 环形缓冲区
 *  - 单位：均值样本
 *  - 只维护 head，无 tail
 *  - 语义：始终保留最近 ADC_BUFF_SIZE 个均值，满则覆盖最旧
 * ============================================================ */

#if (ADC_BUFF_SIZE & (ADC_BUFF_SIZE - 1)) != 0
#error "ADC_BUFF_SIZE must be power of 2"
#endif

#define ADC_MASK (ADC_BUFF_SIZE - 1)

static xdata uint16_t s_adc_buf[ADC_CHANNEL_COUNT][ADC_BUFF_SIZE];
static data volatile uint8_t s_adc_head[ADC_CHANNEL_COUNT];
static data volatile uint16_t s_adc_last[ADC_CHANNEL_COUNT];
static data uint16_t s_adc_acc[ADC_CHANNEL_COUNT];
static data uint8_t s_adc_count[ADC_CHANNEL_COUNT];
static data uint8_t s_active_channel;

static uint8_t adc_hw_channel(adc_channel_t channel) { return (channel == ADC_CHANNEL_P54) ? ADC_CH_P54 : ADC_CH_P55; }

/* ============================================================
 * 初始化
 * ============================================================ */

void adc_init(void) {
  uint8_t channel;
  uint8_t i;

  for (channel = 0; channel < ADC_CHANNEL_COUNT; channel++) {
    s_adc_head[channel] = 0;
    s_adc_last[channel] = 0;
    s_adc_acc[channel] = 0;
    s_adc_count[channel] = 0;
    for (i = 0; i < ADC_BUFF_SIZE; i++) {
      s_adc_buf[channel][i] = 0;
    }
  }
  s_active_channel = ADC_CHANNEL_P54;

  P5M0 &= (uint8_t)~0x30;
  P5M1 |= (uint8_t)0x30;

  ADCTIM = 0x3f;
  ADCCFG = 0x0F;
  ADC_CONTR = ADCCONTR_POWER | (ADC_CH_P54 & ADCCONTR_CH_MASK);
  delay_ms(1);
  ADC_CONTR &= (uint8_t)~ADCCONTR_FLAG;
  EADC = 1;
}

/* ============================================================
 * 启动一次转换
 *  - 由 Timer0 ISR 每 125us 调用
 *  - 含 START 卡死守护
 *  - 同级中断（T0 / ADC）不互相嵌套，ADC_CONTR 的 RMW 天然原子
 * ============================================================ */

void adc_start(void) {
  static uint8_t stuck = 0;

  if (ADC_CONTR & ADCCONTR_FLAG) {
    return;
  }

  if (ADC_CONTR & ADCCONTR_START) {
    if (++stuck > ADC_START_STUCK_LIMIT) {
      ADC_CONTR = 0;
      _nop_();
      _nop_();
      s_adc_acc[s_active_channel] = 0;
      s_adc_count[s_active_channel] = 0;
      ADC_CONTR = ADCCONTR_POWER | (adc_hw_channel((adc_channel_t)s_active_channel) & ADCCONTR_CH_MASK);
      stuck = 0;
    }
    return;
  }

  stuck = 0;
  ADC_CONTR |= ADCCONTR_START;
}

/* ============================================================
 * 查询接口
 * ============================================================ */

uint16_t adc_last_value(adc_channel_t channel) {
  uint16_t v;
  bit e_save;

  if (channel >= ADC_CHANNEL_COUNT) return 0;
  e_save = EADC;
  EADC = 0;
  v = s_adc_last[channel];
  EADC = e_save;
  return v;
}

/* ============================================================
 * 快照读取：拷贝指定通道最近 len 个均值样本
 *  - 暂停 Timer0 与 ADC 中断，避免快照期间覆盖源区或启动新转换
 *  - 关中断时长：len 个样本 × 约 5 周期拷贝
 *    最大 128 × 5 = 640 周期 @24MHz ≈ 27us < 125us 节拍
 *    （ADC 硬件转换异步完成时，待处理中断会在恢复后响应）
 * ============================================================ */

void adc_read(adc_channel_t channel, uint16_t* buf, uint8_t len) {
  uint8_t head;
  uint8_t start;
  uint16_t i;
  bit et0_save;
  bit eadc_save;

  if (channel >= ADC_CHANNEL_COUNT || buf == NULL || len == 0) return;
  if (len > ADC_BUFF_SIZE) len = ADC_BUFF_SIZE;

  et0_save = ET0;
  eadc_save = EADC;
  ET0 = 0;
  EADC = 0;

  head = s_adc_head[channel];
  start = (uint8_t)((head - len) & ADC_MASK);

  for (i = 0; i < len; i++) {
    buf[i] = s_adc_buf[channel][start];
    start = (uint8_t)((start + 1) & ADC_MASK);
  }

  EADC = eadc_save;
  ET0 = et0_save;
  return;
}

/* ============================================================
 * ADC 中断服务程序
 * ============================================================ */

void ADC_ISR(void) interrupt ADC_VECTOR {
  if (ADC_CONTR & ADCCONTR_FLAG) {
    uint8_t channel = s_active_channel;
    uint16_t sample;

    ADC_CONTR &= (uint8_t)~ADCCONTR_FLAG;

    sample = (uint16_t)(((unsigned int)ADC_RES << 2) | (ADC_RESL >> 6));
    s_adc_acc[channel] += sample;
    s_adc_count[channel]++;

    if (s_adc_count[channel] >= ADC_AVG_WIN) {
      s_adc_last[channel] = (uint16_t)(s_adc_acc[channel] >> 2);

      s_adc_buf[channel][s_adc_head[channel]] = s_adc_last[channel];
      s_adc_head[channel] = (uint8_t)((s_adc_head[channel] + 1) & ADC_MASK);

      s_adc_acc[channel] = 0;
      s_adc_count[channel] = 0;
    }

    s_active_channel = (uint8_t)(channel ^ 1);
    ADC_CONTR = ADCCONTR_POWER | (adc_hw_channel((adc_channel_t)s_active_channel) & ADCCONTR_CH_MASK);
  }
}