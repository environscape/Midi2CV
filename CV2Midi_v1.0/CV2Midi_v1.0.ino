#include <MIDI.h>

// 定义MIDI消息类型枚举
enum MidiMessageType {
  CONTROL_CHANGE,  // CC消息
  NOTE_NUMBER,     // 音符编号配置
  NOTE_ON_OFF,     // 音符开关控制
  CLOCK_START,     // 时钟开始
  CLOCK_STOP,      // 时钟停止
  CLOCK_TICK       // 时钟节拍
};

// 配置数组 [输入编号][配置参数]
// 参数含义: [MIDI通道(1-16), MIDI消息类型, 数据1(CC编号/音符基准编号), 数据2(默认速度/值)]
const byte cvConfig[8][4] = {
  { 3, NOTE_NUMBER, 20, 64 },    // CV0: 通道1, CC消息, CC编号20, 默认值64
  { 3, NOTE_ON_OFF, 60, 100 },   // CV1: 通道2, 音符编号, 基准音符编号60(C4), 默认速度100
  { 4, NOTE_NUMBER, 0, 0 },      // CV2: 通道2, 音符开关
  { 4, NOTE_ON_OFF, 7, 64 },     // CV3: 通道3, CC消息, CC编号7(音量), 默认值64
  { 10, NOTE_NUMBER, 1, 0 },     // CV4: 通道4, CC消息, CC编号1(调制轮), 默认值0
  { 10, NOTE_ON_OFF, 0, 0 },     // CV5: 通道1, 时钟开始
  { 3, CONTROL_CHANGE, 1, 64 },  // CV6: 通道1, 时钟停止
  { 4, CONTROL_CHANGE, 2, 64 }   // CV7: 通道1, 时钟节拍
};
// const byte cvConfig[8][4] = {
//   { 1, CONTROL_CHANGE, 20, 64 },  // CV0: 通道1, CC消息, CC编号20, 默认值64
//   { 2, NOTE_NUMBER, 60, 100 },    // CV1: 通道2, 音符编号, 基准音符编号60(C4), 默认速度100
//   { 2, NOTE_ON_OFF, 0, 0 },       // CV2: 通道2, 音符开关
//   { 3, CONTROL_CHANGE, 7, 64 },   // CV3: 通道3, CC消息, CC编号7(音量), 默认值64
//   { 4, CONTROL_CHANGE, 1, 0 },    // CV4: 通道4, CC消息, CC编号1(调制轮), 默认值0
//   { 1, CLOCK_START, 0, 0 },       // CV5: 通道1, 时钟开始
//   { 1, CLOCK_STOP, 0, 0 },        // CV6: 通道1, 时钟停止
//   { 1, CLOCK_TICK, 0, 0 }         // CV7: 通道1, 时钟节拍
// };

/******************************************以下程序不要改动***********************************************/

// MIDI设置
MIDI_CREATE_DEFAULT_INSTANCE();

// 音符状态结构体
struct NoteState {
  byte number;      // 当前音符编号
  bool isOn;        // 音符是否开启
  byte velocity;    // 音符速度
  byte lastNumber;  // 上次发送的音符编号（用于优化）
};

// CC项结构体（存储单个CC的编号和值）
struct CCItem {
  byte cc;     // CC编号
  byte value;  // CC值
};

// CC消息缓存结构体（每个通道最多存储8个CC项）
struct CCCache {
  CCItem items[8];  // CC项数组
  byte count;       // 已存储的CC数量
};

// 模拟输入值和状态存储
int analogValues[8];
bool lastStates[8] = { false };  // 用于存储上次状态，处理音符和时钟消息

// 音符状态管理数组 (最多支持16个MIDI通道)
NoteState noteStates[16];

// CC消息缓存数组 (最多支持16个MIDI通道)
CCCache ccCaches[16];

// 时钟消息缓存（记录每个时钟消息的上次状态）
bool lastClockStates[3] = { false, false, false };  // START, STOP, TICK

void setup() {
  // 初始化MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // 初始化音符状态
  for (int i = 0; i < 16; i++) {
    noteStates[i].number = 60;
    noteStates[i].isOn = false;
    noteStates[i].velocity = 100;
    noteStates[i].lastNumber = 0;
  }

  // 初始化CC缓存
  for (int ch = 0; ch < 16; ch++) {
    ccCaches[ch].count = 0;
    for (int i = 0; i < 8; i++) {
      ccCaches[ch].items[i].cc = 0;
      ccCaches[ch].items[i].value = 0;
    }
  }
}

void loop() {
  // 读取所有模拟输入引脚
  readAnalogInputs();

  // 处理并发送MIDI消息
  processAndSendMidi();

  // 延时避免过于频繁的采样
  // delay(1);
}

// 读取所有模拟输入引脚
void readAnalogInputs() {
  for (int i = 0; i < 8; i++) {
    analogValues[i] = analogRead(i);
  }
}

// 在CC缓存中查找指定CC编号的索引，未找到返回-1
int findCCIndex(byte channel, byte cc) {
  CCCache* cache = &ccCaches[channel - 1];
  for (int i = 0; i < cache->count; i++) {
    if (cache->items[i].cc == cc) {
      return i;
    }
  }
  return -1;
}

// 添加或更新CC缓存中的值
void updateCCCache(byte channel, byte cc, byte value) {
  CCCache* cache = &ccCaches[channel - 1];
  int index = findCCIndex(channel, cc);

  if (index >= 0) {
    // 更新现有CC值
    cache->items[index].value = value;
  } else if (cache->count < 8) {
    // 添加新CC项（仅当未超过最大数量时）
    cache->items[cache->count].cc = cc;
    cache->items[cache->count].value = value;
    cache->count++;
  }
  // 超过8个时忽略（可根据需要实现替换策略）
}

// 获取CC缓存中的值，未找到返回0
byte getCCValue(byte channel, byte cc) {
  int index = findCCIndex(channel, cc);
  return (index >= 0) ? ccCaches[channel - 1].items[index].value : 0;
}

// 处理并发送MIDI消息
void processAndSendMidi() {
  for (int i = 0; i < 8; i++) {
    byte channel = cvConfig[i][0];
    byte messageType = cvConfig[i][1];
    byte data1 = cvConfig[i][2];
    byte data2 = cvConfig[i][3];

    bool currentState = false;
    byte value = 0;

    switch (messageType) {
      case CONTROL_CHANGE:
        // 将模拟值映射到MIDI范围(0-127)
        value = map(analogValues[i], 0, 1023, 0, 127);

        // 仅在值变化时发送CC消息
        byte lastValue = getCCValue(channel, data1);
        if (value != lastValue) {
          MIDI.sendControlChange(data1, value, channel);
          updateCCCache(channel, data1, value);
        }
        break;

      case NOTE_NUMBER:
        // 将模拟值映射到音符编号范围(0-127)
        value = map(analogValues[i], 0, 1023, data1 - 24, data1 + 24);
        value = constrain(value, 0, 127);
        noteStates[channel - 1].number = value;
        break;

      case NOTE_ON_OFF:
        // 音符开关基于固定阈值512触发
        currentState = analogValues[i] > 512;
        if (currentState != lastStates[i]) {
          NoteState* note = &noteStates[channel - 1];
          if (currentState && (!note->isOn || note->number != note->lastNumber)) {
            MIDI.sendNoteOn(note->number, note->velocity, channel);
            note->isOn = true;
            note->lastNumber = note->number;
          } else if (!currentState && note->isOn) {
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
        int clockType = messageType - CLOCK_START;
        if (currentState != lastClockStates[clockType]) {
          // if (messageType == CLOCK_START && currentState) MIDI.sendRealTime(MIDI_START);
          // else if (messageType == CLOCK_STOP && !currentState) MIDI.sendRealTime(MIDI_STOP);
          // else if (messageType == CLOCK_TICK && currentState) MIDI.sendRealTime(MIDI_CLOCK);
          lastClockStates[clockType] = currentState;
        }
        break;
    }
  }
}