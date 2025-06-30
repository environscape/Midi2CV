#include <MIDI.h>

// MIDI设置
MIDI_CREATE_DEFAULT_INSTANCE();

// 定义MIDI消息类型枚举
enum MidiMessageType {
  CONTROL_CHANGE,  // CC消息
  NOTE_NUMBER,     // 音符编号配置
  NOTE_ON_OFF,     // 音符开关控制
  CLOCK_START,     // 时钟开始
  CLOCK_STOP,      // 时钟停止
  CLOCK_TICK       // 时钟节拍
};

// 音符状态结构体
struct NoteState {
  byte number;    // 当前音符编号
  bool isOn;      // 音符是否开启
  byte velocity;  // 音符速度
};

// 配置数组 [输入编号][配置参数]
// 参数含义: [MIDI消息类型, MIDI通道(1-16), 数据1(CC编号/音符基准编号), 数据2(默认速度/值)]
const byte cvConfig[8][4] = {
  { CONTROL_CHANGE, 1, 20, 64 },  // 输入0: CC消息, 通道1, CC编号20, 默认值64
  { NOTE_NUMBER, 2, 60, 100 },    // 输入1: 音符编号, 通道2, 基准音符编号60(C4), 默认速度100
  { NOTE_ON_OFF, 2, 0, 0 },       // 输入2: 音符开关, 通道2
  { CONTROL_CHANGE, 3, 7, 64 },   // 输入3: CC消息, 通道3, CC编号7(音量), 默认值64
  { CONTROL_CHANGE, 4, 1, 0 },    // 输入4: CC消息, 通道4, CC编号1(调制轮), 默认值0
  { CLOCK_START, 10, 0, 0 },      // 输入5: 时钟开始, 通道10
  { CLOCK_STOP, 10, 0, 0 },       // 输入6: 时钟停止, 通道10
  { CLOCK_TICK, 10, 0, 0 }        // 输入7: 时钟节拍, 通道10
};

// 模拟输入值和状态存储
int analogValues[8];
bool lastStates[8] = { false };  // 用于存储上次状态，处理音符和时钟消息

// 音符状态管理数组 (最多支持16个MIDI通道)
NoteState noteStates[16];

void setup() {
  // 初始化MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // 初始化音符状态
  for (int i = 0; i < 16; i++) {
    noteStates[i].number = 60;  // 默认音符为C4
    noteStates[i].isOn = false;
    noteStates[i].velocity = 100;  // 默认速度为100
  }
}

void loop() {
  // 读取所有模拟输入引脚
  readAnalogInputs();

  // 处理并发送MIDI消息
  processAndSendMidi();

  // 延时避免过于频繁的采样
  delay(10);
}

// 读取所有模拟输入引脚
void readAnalogInputs() {
  for (int i = 0; i < 8; i++) {
    analogValues[i] = analogRead(i);
  }
}

// 处理并发送MIDI消息
void processAndSendMidi() {
  for (int i = 0; i < 8; i++) {
    byte messageType = cvConfig[i][0];
    byte channel = cvConfig[i][1];
    byte data1 = cvConfig[i][2];  // CC编号/音符基准编号
    byte data2 = cvConfig[i][3];  // 默认速度/值

    bool currentState = false;
    byte value = 0;

    // 根据消息类型处理输入
    switch (messageType) {
      case CONTROL_CHANGE:
        // 将模拟值映射到MIDI范围(0-127)
        value = map(analogValues[i], 0, 1023, 0, 127);
        MIDI.sendControlChange(data1, value, channel);
        break;

      case NOTE_NUMBER:
        // 将模拟值映射到音符编号范围(0-127)
        // 使用data1作为基准音符，允许偏移±24半音
        value = map(analogValues[i], 0, 1023, data1 - 24, data1 + 24);
        value = constrain(value, 0, 127);  // 确保音符编号在有效范围内

        // 更新对应通道的音符编号
        noteStates[channel - 1].number = value;
        break;

      case NOTE_ON_OFF:
        // 音符开关基于固定阈值512触发
        currentState = analogValues[i] > 512;
        if (currentState != lastStates[i]) {
          NoteState* note = &noteStates[channel - 1];

          if (currentState) {
            // 音符开启
            MIDI.sendNoteOn(note->number, note->velocity, channel);
            note->isOn = true;
          } else {
            // 音符关闭
            MIDI.sendNoteOff(note->number, note->velocity, channel);
            note->isOn = false;
          }

          lastStates[i] = currentState;
        }
        break;

      case CLOCK_START:
      case CLOCK_STOP:
      case CLOCK_TICK:
        // 时钟消息基于固定阈值512触发
        currentState = analogValues[i] > 512;
        if (currentState != lastStates[i]) {
          if (messageType == CLOCK_START && currentState) {
            MIDI.sendRealTime(MIDI_START);
          } else if (messageType == CLOCK_STOP && !currentState) {
            MIDI.sendRealTime(MIDI_STOP);
          } else if (messageType == CLOCK_TICK && currentState) {
            MIDI.sendRealTime(MIDI_CLOCK);
          }
          lastStates[i] = currentState;
        }
        break;
    }
  }
}