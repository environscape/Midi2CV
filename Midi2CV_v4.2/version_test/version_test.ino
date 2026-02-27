void setup() {
  Serial.begin(31250);        //与midi通信的波特率相同
  pinMode(A0, INPUT_PULLUP);  //拉高以便检测电路板版本 软件>=4.2的版本都会拉高这个电平
  pinMode(A1, INPUT);         //A1引脚用于检测调谐情况
}

//测试a0引脚的情况
// 电路板v4.0版本以下 DA0为高电平
// 电路板v4.0版本及以上 DA0为低电平
void loop() {
  Serial.print(" DA0=");
  Serial.print(digitalRead(A0));
  Serial.print(" A0=");
  Serial.print(analogRead(A0));
  Serial.print(" A1=");
  Serial.print(analogRead(A1));
  Serial.print("\n");
}