#ifndef __TIMER_H__
#define __TIMER_H__

#include "config.h"

extern data volatile uint32_t g_tick_ms;

void timer_init(void);
uint32_t timer_get_ms(void);

#endif /* __TIMER_H__ */