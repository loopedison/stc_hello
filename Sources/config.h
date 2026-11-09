#ifndef __CONFIG_H__
#define __CONFIG_H__

/* ============================================================
 * STC8G1K08A 非阻塞框架 全局配置
 * ============================================================ */

#include <float.h>
#include <intrins.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "STC8G.H"
#include "def.h"

#define __ENCODING "UTF-8"  // DO NOT DELETE or MODIFY
#define UNUSED(x) if (x)    //  DO NOT DELETE or MODIFY

// ============================================================
// 系统时钟
// ============================================================

#define SYSCLK 24000000UL
#define FOSC 24000000UL
#define MAIN_Fosc FOSC

// ============================================================
// 全局缓冲区配置以及部分宏定义
// ============================================================

#define UART_BAUDRATE 115200UL  // todo 目前为硬编码波特率，不支持修改
#define UART_TX_BUFF_SIZE 128   // TX 缓冲区大小（必须为 2 的幂，且 <= 128）
#define UART_RX_BUFF_SIZE 32    // RX 缓冲区大小（必须为 2 的幂，且 <= 128）
#define SHELL_LINE_SIZE 16      // SHELL 单行命令长度（<= 32）
#define ADC_BUFF_SIZE 128       // 每通道 ADC 样本数（必须为 2 的幂，且 <= 128）

// ============================================================
// 中断向量号（严格参考自芯片手册）
// ============================================================

#define TMR0_VECTOR 1
#define UART1_VECTOR 4
#define ADC_VECTOR 5

// ============================================================
// 阻塞延时函数
// ============================================================

void delay_ms(uint16_t ms);

#endif /* __CONFIG_H__ */
