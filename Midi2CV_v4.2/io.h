const byte LDAC = 9;  //SPI trans setting

//DAC_CV output
void OUT_VOCT1(int cv) {
  digitalWrite(LDAC, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer((cv >> 8) | 0x30);  // H0x30=OUTA/1x
  SPI.transfer(cv & 0xff);
  digitalWrite(SS, HIGH);
  digitalWrite(LDAC, LOW);
}

//DAC_CV2 output
void OUT_VOCT2(int cv2) {
  digitalWrite(LDAC, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer((cv2 >> 8) | 0xB0);  // H0xB0=OUTB/1x
  SPI.transfer(cv2 & 0xff);
  digitalWrite(SS, HIGH);
  digitalWrite(LDAC, LOW);
}

void OUT_PWM(int pin, int cc_value) {  //pwm 0-255
  analogWrite(pin, cc_value << 1);
}

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