#include <MIDI.h>

enum MidiMessageType {
  CONTROL_CHANGE,  // CC消息
  NOTE_NUMBER,     // 音符编号配置
  NOTE_ON_OFF,     // 音符开关控制
  NOTE_VEL,        // 音符力度
};
int CVMode = 0;  // 当前模式(0-3)
// 四种模式的CV配置表：[通道, 消息类型, 参数值]
const byte CVConfig0[8][3] = {
  { 3, NOTE_NUMBER, 0 },      // CV0: 通道3, 音符编号
  { 3, NOTE_ON_OFF, 0 },      // CV1: 通道3, 音符开关
  { 3, NOTE_VEL, 0 },         // CV2: 通道3, 音符力度
  { 3, CONTROL_CHANGE, 54 },  // CV3: 通道3, CC编号1
  { 3, CONTROL_CHANGE, 56 },  // CV4: 通道3, CC编号2
  { 3, CONTROL_CHANGE, 74 },  // CV5: 通道3, CC编号3
  { 3, CONTROL_CHANGE, 71 },  // CV6: 通道3, CC编号4
  { 3, CONTROL_CHANGE, 93 }   // CV7: 通道4, CC编号5
};

const byte CVConfig1[8][3] = {
  { 4, NOTE_NUMBER, 0 },     // CV0: 通道4, 音符编号
  { 4, NOTE_ON_OFF, 0 },     // CV1: 通道4, 音符开关
  { 4, NOTE_VEL, 0 },        // CV2: 通道4, 音符力度
  { 4, CONTROL_CHANGE, 6 },  // CV3: 通道4, CC编号6
  { 4, CONTROL_CHANGE, 7 },  // CV4: 通道4, CC编号7
  { 4, CONTROL_CHANGE, 8 },  // CV5: 通道4, CC编号8
  { 4, CONTROL_CHANGE, 9 },  // CV6: 通道4, CC编号9
  { 4, CONTROL_CHANGE, 10 }  // CV7: 通道5, CC编号10
};

const byte CVConfig2[8][3] = {
  { 3, NOTE_NUMBER, 0 },      // CV0: 通道3, 音符编号
  { 3, NOTE_ON_OFF, 0 },      // CV1: 通道3, 音符开关
  { 3, NOTE_VEL, 0 },         // CV2: 通道3, 音符力度
  { 3, CONTROL_CHANGE, 80 },  // CV3: 通道3, CC编号1
  { 3, CONTROL_CHANGE, 81 },  // CV4: 通道3, CC编号2
  { 3, CONTROL_CHANGE, 82 },  // CV5: 通道3, CC编号3
  { 3, CONTROL_CHANGE, 83 },  // CV6: 通道3, CC编号4
  { 3, CONTROL_CHANGE, 84 }   // CV7: 通道4, CC编号5
};

const byte CVConfig3[8][3] = {
  { 10, NOTE_NUMBER, 0 },      // CV0: 通道6, 音符编号
  { 10, NOTE_ON_OFF, 0 },      // CV1: 通道6, 音符开关
  { 10, NOTE_VEL, 0 },         // CV2: 通道6, 音符力度
  { 10, CONTROL_CHANGE, 14 },  // CV3: 通道6, CC编号16
  { 10, CONTROL_CHANGE, 15 },  // CV4: 通道6, CC编号17
  { 10, CONTROL_CHANGE, 46 },  // CV5: 通道6, CC编号18
  { 10, CONTROL_CHANGE, 47 },  // CV6: 通道6, CC编号19
  { 10, CONTROL_CHANGE, 50 }   // CV7: 通道7, CC编号20
};

MIDI_CREATE_DEFAULT_INSTANCE();


// 当前使用的配置表指针
const byte (*currentConfig)[8][3] = &CVConfig0;

// MIDI消息缓冲区
struct MidiBuffer {
  bool noteOn = false;   // 音符状态
  byte noteNumber = 60;  // 默认中央C (C4)
  byte velocity = 100;   // 默认力度
  byte channel = 1;      // 默认通道
};

MidiBuffer midiBuffer;  // MIDI消息缓冲区

// int ANALOG_MAX_VALUE = 1023   ; //Arduino
int ANALOG_MAX_VALUE = 4095;  //Lgt8f328p

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);  // 初始化MIDI
  // Serial.begin(31250);            // 初始化串口通信（用于调试）

  pinMode(11, INPUT_PULLUP);  // D11按钮引脚(上拉输入)
  pinMode(12, OUTPUT);        // D12输出引脚
  pinMode(13, OUTPUT);        // D13输出引脚
}

void loop() {
  setMode();          // 处理模式切换
  view();             // 更新输出引脚状态并选择配置表
  processCVInputs();  // 处理CV输入并发送MIDI消息
}

// 处理模式切换逻辑
int d11 = 0;  // 开关临时变量
void setMode() {
  if (digitalRead(11) == LOW && d11 == 0) {  // 按钮按下
    d11 = 1;
    CVMode = (CVMode + 1) % 4;  // 循环切换0-3
    // 模式切换时关闭所有音符
    for (int ch = 1; ch <= 16; ch++) {
      MIDI.sendControlChange(123, 0, ch);
    }
    midiBuffer.noteOn = false;
  }
  if (digitalRead(11) == HIGH && d11 == 1) {  // 按钮释放
    d11 = 0;
  }
}

// 根据当前模式更新D12和D13状态并选择配置表
void view() {
  switch (CVMode) {
    case 0:
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      currentConfig = &CVConfig0;
      break;
    case 1:
      digitalWrite(12, HIGH);
      digitalWrite(13, LOW);
      currentConfig = &CVConfig1;
      break;
    case 2:
      digitalWrite(12, LOW);
      digitalWrite(13, HIGH);
      currentConfig = &CVConfig2;
      break;
    case 3:
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
      currentConfig = &CVConfig3;
      break;
  }
}

// 处理所有CV输入并发送相应的MIDI消息
void processCVInputs() {
  bool newNoteOnState = false;

  // 读取并处理A0-A7输入
  for (int i = 0; i < 8; i++) {
    int cvValue = analogRead(i);
    byte channel = (*currentConfig)[i][0];
    MidiMessageType messageType = static_cast<MidiMessageType>((*currentConfig)[i][1]);
    byte param = (*currentConfig)[i][2];

    switch (messageType) {
      case NOTE_NUMBER:
        // 直接映射CV值到MIDI音符编号 (C0-C8范围)
        midiBuffer.noteNumber = constrain(map(cvValue, 0, ANALOG_MAX_VALUE, 12, 108), 0, 127);
        break;

      case NOTE_ON_OFF:
        // 音符开关控制
        newNoteOnState = (cvValue > ANALOG_MAX_VALUE / 2);
        if (newNoteOnState != midiBuffer.noteOn) {
          midiBuffer.noteOn = newNoteOnState;
          midiBuffer.channel = channel;

          if (newNoteOnState) {
            // 发送Note On消息
            MIDI.sendNoteOn(midiBuffer.noteNumber, midiBuffer.velocity, channel);
          } else {
            // 发送All Notes Off消息
            MIDI.sendControlChange(123, 0, channel);
          }
        }
        break;

       
      case NOTE_VEL:
        // 直接映射CV值到MIDI力度 (30-127范围)
        midiBuffer.velocity = constrain(map(cvValue, 0, ANALOG_MAX_VALUE, 30, 127), 0, 127);
        break;
        
      case CONTROL_CHANGE: 
        // 直接发送CC消息
        byte ccValue = map(cvValue, 0, ANALOG_MAX_VALUE, 0, 127);
        MIDI.sendControlChange(param, ccValue, channel);
        break;
    }
  }
}