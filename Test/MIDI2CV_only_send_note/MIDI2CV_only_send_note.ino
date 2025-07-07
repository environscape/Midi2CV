#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const int NOTE_C3 = 48;               // C3音符的MIDI编号
const int CHANNEL = 3;                // MIDI通道3
const int VELOCITY = 126;             // 力度值
const unsigned long INTERVAL = 1000;  // 1秒间隔

// 新增模式控制相关定义
const int MODE_BUTTON = 11;               // 模式切换按钮引脚(D11)
const int OUTPUT_D12 = 12;                // D12输出引脚
const int OUTPUT_D13 = 13;                // D13输出引脚
int mode = 0;                             // 当前模式(0-3)
int d11 = 0;                              // 开关临时
int lastButtonState = HIGH;               // 上次按钮状态(初始为高，因按钮拉高)
unsigned long lastDebounceTime = 0;       // 按钮防抖时间记录
const unsigned long DEBOUNCE_DELAY = 50;  // 防抖延迟(ms)

unsigned long previousMillis = 0;  // 上次状态切换时间
bool noteState = false;            // 当前音符状态（ON/OFF）

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);  // 初始化MIDI
  Serial.begin(31250);            // 初始化串口通信（用于调试）
  // Serial.println("A0\tA1\tA2\tA3\tA4\tA5\tA6\tA7");  // 表头  打印日志有可能影响mid信号! lecheng

  // 新增：设置模式控制引脚
  pinMode(MODE_BUTTON, INPUT_PULLUP);  // D11按钮引脚(上拉输入)
  pinMode(OUTPUT_D12, OUTPUT);         // D12输出引脚
  pinMode(OUTPUT_D13, OUTPUT);         // D13输出引脚
  view();                  // 初始化输出引脚状态
}

void loop() {
  // 新增：处理模式切换
  handleModeSwitch();

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

// 新增：处理模式切换逻辑
void handleModeSwitch() {
  Serial.print(" d11 ");
  Serial.print(digitalRead(11));
  Serial.print(" d12 ");
  Serial.print(digitalRead(12));
  Serial.print(" d13 ");
  Serial.print(digitalRead(13));
  Serial.print(" mode ");
  Serial.println(mode);


  if (digitalRead(11) == 1 && d11 == 0) {
    d11 = 1;
    mode = (mode + 1) % 4;  // 循环切换0-3
  }
  if (digitalRead(11) == 0 && d11 == 1) {
    d11 = 0;
  }
  view();  // 更新输出引脚状态
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