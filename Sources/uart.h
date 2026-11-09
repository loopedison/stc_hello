#ifndef __UART_H__
#define __UART_H__

#include "config.h"

void uart_init(void);
void uart_send(const uint8_t* dat, uint8_t len);
uint8_t uart_read_byte(uint8_t* b);

uint8_t uart_tx_used(void);
uint8_t uart_rx_used(void);
uint8_t uart_tx_drop(void);
uint8_t uart_rx_drop(void);

#endif /* __UART_H__ */