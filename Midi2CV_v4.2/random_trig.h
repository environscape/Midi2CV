
unsigned long mytimer_start_time = 0;       // 用于记录事件开始时间
unsigned long mytimer_delay_time = 100000;  // 用于记录事件开始时间

void myTimerStart(unsigned long time) {
  mytimer_delay_time = time;
}

void myTimerLoop() {
  if (millis() - mytimer_start_time >= mytimer_delay_time) {  //事件持续10秒钟或以上
    digitalWrite(GATE2_PIN, 0);                               //在这里编写你的代码来响应这个事件
  }
}

void myTimerReset() {
  mytimer_start_time = millis();  // 重置开始时间
}

void triggerOn() {
  int randomNum = random(0, 255);
  
  //rand trig
  if (randomNum < 192) {
    OUT_CV1(4095);
    OUT_CV1(0);
  }
  if (randomNum < 128) {
    digitalWrite(GATE1_PIN, 1);
  }
  if (randomNum < 64) {
    OUT_CV2(4095);
  }

  //rand gate length
  digitalWrite(GATE2_PIN, 1);
  myTimerStart(random(1, 1024) );//线性改绿曲线
  // myTimerStart(random(1, 32) * random(1, 32));//幂概率曲线
  myTimerReset();

  //rabd level
  analogWrite(CV1_PIN, randomNum);         //8bit 0-256  2^8
  analogWrite(CV2_PIN, 255 - randomNum);   //inv
  analogWrite(CV3_PIN, (randomNum >> 1));  //7bit 0-128
}

void triggerOff() {
  OUT_CV1(0);
  digitalWrite(GATE1_PIN, 0);
  OUT_CV2(0);
  // digitalWrite(GATE2_PIN, 0);
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
