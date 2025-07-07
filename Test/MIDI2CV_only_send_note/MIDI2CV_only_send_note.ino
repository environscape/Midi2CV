#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const int NOTE_C3 = 48;               // C3音符的MIDI编号
const int CHANNEL = 3;                // MIDI通道3
const int VELOCITY = 126;             // 力度值
const unsigned long INTERVAL = 1000;  // 1秒间隔

unsigned long previousMillis = 0;  // 上次状态切换时间
bool noteState = false;            // 当前音符状态（ON/OFF）

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);  // 初始化MIDI
  Serial.begin(31250);            // 初始化串口通信（用于调试）
  // Serial.println("A0\tA1\tA2\tA3\tA4\tA5\tA6\tA7");  // 表头  打印日志有可能影响mid信号! lecheng
}

void loop() {
  unsigned long currentMillis = millis();

  // 每1秒切换一次音符状态
  if (currentMillis - previousMillis >= INTERVAL) {
    previousMillis = currentMillis;
    noteState = !noteState;  // 切换状态

    if (noteState) {
      // 发送Note On消息
      MIDI.sendNoteOn(NOTE_C3, VELOCITY, CHANNEL);
      Serial.println("Note On: C3 (MIDI #48)");
    } else {
      // 发送Note Off消息
      MIDI.sendNoteOff(NOTE_C3, 0, CHANNEL);
      Serial.println("Note Off: C3 (MIDI #48)");
    }
  }


  //日志：
  // 读取并输出A0-A7的值（制表符分隔，便于在串口监视器中查看）
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