#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

int mode = 0;  // 当前模式(0-3)
int d11 = 0;   // 开关临时变量

enum MidiMessageType {
  CONTROL_CHANGE,  // CC消息
  NOTE_NUMBER,     // 音符编号配置
  NOTE_ON_OFF,     // 音符开关控制
  NOTE_VEL,        // 音符力度
};

// CV配置表：[通道, 消息类型, CC编号]
const byte CVConfig[8][3] = {
  { 3, NOTE_NUMBER, 0 },     // CV0: 通道3, 音符编号
  { 3, NOTE_ON_OFF, 0 },     // CV1: 通道3, 音符开关
  { 3, NOTE_VEL, 0 },        // CV2: 通道3, 音符力度
  { 3, CONTROL_CHANGE, 1 },  // CV3: 通道3, CC编号1
  { 3, CONTROL_CHANGE, 2 },  // CV4: 通道3, CC编号2
  { 3, CONTROL_CHANGE, 3 },  // CV5: 通道3, CC编号3
  { 3, CONTROL_CHANGE, 4 },  // CV6: 通道3, CC编号4
  { 4, CONTROL_CHANGE, 5 }   // CV7: 通道4, CC编号5
};

// MIDI消息缓冲区
struct MidiBuffer {
  bool noteOn = false;   // 音符状态
  byte noteNumber = 60;  // 默认中央C (C4)
  byte velocity = 100;   // 默认力度
  byte channel = 1;      // 默认通道
};

MidiBuffer midiBuffer;  // MIDI消息缓冲区

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);  // 初始化MIDI
  // Serial.begin(31250);            // 初始化串口通信（用于调试）

  pinMode(11, INPUT_PULLUP);  // D11按钮引脚(上拉输入)
  pinMode(12, OUTPUT);        // D12输出引脚
  pinMode(13, OUTPUT);        // D13输出引脚
}

void loop() {
  setMode();          // 处理模式切换
  view();             // 更新输出引脚状态
  processCVInputs();  // 处理CV输入并发送MIDI消息
}

// 处理模式切换逻辑
void setMode() {
  if (digitalRead(11) == LOW && d11 == 0) {  // 按钮按下
    d11 = 1;
    mode = (mode + 1) % 4;  // 循环切换0-3
  }
  if (digitalRead(11) == HIGH && d11 == 1) {  // 按钮释放
    d11 = 0;
  }
}

// 根据当前模式更新D12和D13状态
void view() {
  switch (mode) {
    case 0:
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      break;
    case 1:
      digitalWrite(12, HIGH);
      digitalWrite(13, LOW);
      break;
    case 2:
      digitalWrite(12, LOW);
      digitalWrite(13, HIGH);
      break;
    case 3:
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
      break;
  }
}

// 处理所有CV输入并发送相应的MIDI消息
void processCVInputs() {
  bool newNoteOnState = false;

  // 读取并处理A0-A7输入
  for (int i = 0; i < 8; i++) {
    int cvValue = analogRead(i);
    byte channel = CVConfig[i][0];
    MidiMessageType messageType = static_cast<MidiMessageType>(CVConfig[i][1]);
    byte param = CVConfig[i][2];

    switch (messageType) {
      case NOTE_NUMBER:
        // 直接映射CV值到MIDI音符编号 (C0-C8范围)
        midiBuffer.noteNumber = constrain(map(cvValue, 0, 1023, 12, 108), 0, 127);
        break;

      case NOTE_ON_OFF:
        // 音符开关控制
        newNoteOnState = (cvValue > 512);
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
        midiBuffer.velocity = constrain(map(cvValue, 0, 1023, 30, 127), 0, 127);
        break;

      case CONTROL_CHANGE:
        // 直接发送CC消息
        byte ccValue = map(cvValue, 0, 1023, 0, 127);
        MIDI.sendControlChange(param, ccValue, channel);
        break;
    }
  }
}


/*

  这是我使用arduino nano开发的 cv转midi模块代码   
  现在  我增加了MidiMessageType和 CVConfig配置能力  
  如果配置成NOTE_NUMBER 则可以使当前的analogread控制 下一次sendNoteOn时的序号 需要将0-1023的数值转化成c0-c8对应的数据  并传入
  如果配置成NOTE_ON_OFF  则当该a引脚大于512时 则直接调用sendNoteOn 并将NOTE_NUMBER转化的值直接发送 
  而小于512时则直接调用    MIDI.sendControlChange(123, 0, 3); 注意：控制器编号123代表All Notes Off   
  当配置成NOTE_VEL 跟音符开关一个道理 在下次sendnoteon 传入 
  当配置成CONTROL_CHANGE 则直接读取第三个参数 获取CONTROL_CHANGE对应的编号 
  以上配置信息的第一个参数都是通道 发送的时候不要忘记传入

*/
