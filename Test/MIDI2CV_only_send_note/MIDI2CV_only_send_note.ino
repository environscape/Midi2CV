#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

int mode = 0;  // 当前模式(0-3)
int d11 = 0;   // 开关临时

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);  // 初始化MIDI
  Serial.begin(31250);            // 初始化串口通信（用于调试）
  // Serial.println("A0\tA1\tA2\tA3\tA4\tA5\tA6\tA7");  // 表头  打印日志有可能影响mid信号! lecheng

  pinMode(11, INPUT_PULLUP);  // D11按钮引脚(上拉输入)
  pinMode(12, OUTPUT);        // D12输出引脚
  pinMode(13, OUTPUT);        // D13输出引脚
}

unsigned long previousMillis = 0;  // 上次状态切换时间
bool noteState = false;            // 当前音符状态（ON/OFF）
void loop() {

  setMode();  // 新增：处理模式切换
  view();     // 更新输出引脚状态

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {  // 每1秒切换一次音符状态
    previousMillis = currentMillis;
    noteState = !noteState;  // 切换状态

    if (noteState) {
      MIDI.sendNoteOn(48, 126, 3);  //Note Velocity Channel      // 发送Note On消息
      // Serial.println("Note On: C3 (MIDI #48)");
    } else {
      MIDI.sendNoteOff(48, 0, CHANNEL);  // 发送Note Off消息
      // Serial.println("Note Off: C3 (MIDI #48)");
    }
  }

  // 日志：读取并输出A0-A7的值（制表符分隔，便于在串口监视器中查看）打印日志有可能影响mid信号! lecheng
  // Serial.print(analogRead(A0));
  // Serial.print('\t');
  // Serial.print(analogRead(A1));
  // Serial.print('\t');
  // Serial.print(analogRead(A2));
  // Serial.print('\t');
  // Serial.print(analogRead(A3));
  // Serial.print('\t');
  // Serial.print(analogRead(A4));
  // Serial.print('\t');
  // Serial.print(analogRead(A5));
  // Serial.print('\t');
  // Serial.print(analogRead(A6));
  // Serial.print('\t');
  // Serial.println(analogRead(A7));
}

// 新增：处理模式切换逻辑
void setMode() {
  if (digitalRead(11) == 1 && d11 == 0) {
    d11 = 1;
    mode = (mode + 1) % 4;  // 循环切换0-3
  }
  if (digitalRead(11) == 0 && d11 == 1) {
    d11 = 0;
  }
  // Serial.print(" d11 ");
  // Serial.print(digitalRead(11));
  // Serial.print(" d12 ");
  // Serial.print(digitalRead(12));
  // Serial.print(" d13 ");
  // Serial.print(digitalRead(13));
  // Serial.print(" mode ");
  // Serial.println(mode);
}

// 新增：根据当前模式更新D12和D13状态
void view() {
  switch (mode) {
    case 0:
      digitalWrite(OUTPUT_D12, HIGH);
      digitalWrite(OUTPUT_D13, HIGH);
      break;
    case 1:
      digitalWrite(OUTPUT_D12, HIGH);
      digitalWrite(OUTPUT_D13, LOW);
      break;
    case 2:
      digitalWrite(OUTPUT_D12, LOW);
      digitalWrite(OUTPUT_D13, HIGH);
      break;
    case 3:
      digitalWrite(OUTPUT_D12, LOW);
      digitalWrite(OUTPUT_D13, LOW);
      break;
  }
}