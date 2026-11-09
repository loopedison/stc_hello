#include "pwm.h"

#include "config.h"

void pwm_init(void) {
  P3M0 |= 0x04;
  P3M1 &= ~0x04;

  P_SW1 &= ~0x30;

  CMOD = 0x00;
  PCA_PWM0 = 0x00;
  CCAPM0 = 0x42;
  CCAP0H = 0x00;
  CCAP0L = 0x00;
  CR = 1;
}

void pwm_set_duty(uint8_t duty) { CCAP0H = duty; }
uint8_t pwm_get_duty(void) { return CCAP0H; }
