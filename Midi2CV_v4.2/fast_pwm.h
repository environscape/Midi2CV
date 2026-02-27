// 配置 D3 (Timer 2) 为快速 PWM 模式
void setFastPWM() {//pwm频率约为62.5khz
  TCCR2A = (TCCR2A & 0xF0) | 0x03;  // 快速 PWM 模式，非反转输出
  TCCR2B = (TCCR2B & 0xF8) | 0x01;  // 预分频因子为 1

  // 配置 D5 和 D6 (Timer 0) 为快速 PWM 模式
  TCCR0A = (TCCR0A & 0xF0) | 0x03;  // 快速 PWM 模式，非反转输出
  TCCR0B = (TCCR0B & 0xF8) | 0x01;  // 预分频因子为 1
}

// 恢复 D3 (Timer 2) 为默认 PWM 模式
void restoreDefaultPWM() {
  TCCR2A = (TCCR2A & 0xF0) | 0x01;  // 相位正确 PWM 模式，非反转输出
  TCCR2B = (TCCR2B & 0xF8) | 0x01;  // 预分频因子为 1

  // 恢复 D5 和 D6 (Timer 0) 为默认 PWM 模式
  TCCR0A = (TCCR0A & 0xF0) | 0x01;  // 相位正确 PWM 模式，非反转输出
  TCCR0B = (TCCR0B & 0xF8) | 0x01;  // 预分频因子为 1
}