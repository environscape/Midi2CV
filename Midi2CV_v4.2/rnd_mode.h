unsigned long timer_start_time = 0;  // 用于记录事件开始时间


void timerReset() {
  timer_start_time = millis();  // 重置开始时间
}

unsigned long mytimer_start_time = 0;       // 用于记录事件开始时间
unsigned long mytimer_delay_time = 100000;  // 用于记录事件开始时间

void myTimerStart(unsigned long time) {
  mytimer_delay_time = time;
}

void myTimerLoop() {
  if (millis() - mytimer_start_time >= mytimer_delay_time) {  //事件持续10秒钟或以上
    OUT_PWM(CV2_PIN, 0);                                      //在这里编写你的代码来响应这个事件
  }
}

void myTimerReset() {
  mytimer_start_time = millis();  // 重置开始时间
}

void triggerOn() {
  int randomNum = random(0, 255);

  //gate>trig
  OUT_VOCT1(4095);
  OUT_VOCT1(0);
  //gate>75%gate
  if (randomNum < 192) {  //
    digitalWrite(GATE1_PIN, 1);
  }
  //gate>50%gate
  if (randomNum < 128) {
    OUT_PWM(CV1_PIN, 127);
  }
  //gate>25%gate
  if (randomNum < 64) {
    OUT_VOCT2(4095);
  }
  //gate>12%gate
  if (randomNum < 32) {
    digitalWrite(GATE2_PIN, 1);
  }
  //rand gate length
  OUT_PWM(CV2_PIN, 127);
  myTimerStart(random(1, 1024));  //线性改绿曲线  // myTimerStart(random(1, 32) * random(1, 32));//幂概率曲线
  myTimerReset();
  //RND level
  analogWrite(CV3_PIN, randomNum);  //inv
}

void triggerOff() {
  OUT_VOCT1(0);
  digitalWrite(GATE1_PIN, 0);
  OUT_PWM(CV1_PIN, 0);
  OUT_VOCT2(0);
  digitalWrite(GATE2_PIN, 0);

}

int trig_clk_in = 0;

void triggerListener() {  //In the loop
  // Serial.println(" triggerListener");
  pinMode(CLOCK_PIN, INPUT);  //CLK_OUT

  if (digitalRead(CLOCK_PIN) == 1 && trig_clk_in == 0) {
    trig_clk_in = 1;
    triggerOn();
  }
  if (digitalRead(CLOCK_PIN) == 0 && trig_clk_in == 1) {
    trig_clk_in = 0;
    triggerOff();
  }

  myTimerLoop();
}



void timerLoop() {
  if (millis() - timer_start_time >= 10000) {  //事件持续10秒钟或以上
    // Serial.println("random mode ");

    restoreDefaultPWM();  //恢复pwm
    enable_rand_trig = 1;
    triggerListener();
  }
}