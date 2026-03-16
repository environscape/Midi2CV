#define CONFIG1_PIN 10  //配置1
#define CONFIG2_PIN 12  //配置2
#define CLOCK_PIN 11    //CLK
//D2-D8 Gate Out
#include <MIDI.h>
#include <SPI.h>  //DAC通信用
#include "random_trig.h"


const byte LDAC = 9;  //SPI trans setting

byte ch1 = 1;

int note_no[8] = {};

// int p3 = 0, p5 = 0, p6 = 0;
// int note4[4] = { 0, 0, 0, 0 };

byte clock_count = 0;  //clock计数器
byte clock_max = 24;   //clock分辨率
int clock_rate = 0;    //Clock速率
int clock_div = 1;     //Clock div 特殊用途

byte cc_mode = 0;          //用于更改当前cc映射模式
int enable_rand_trig = 0;  //随机触发功能: 每当收到midi信号时 就禁用随机触发功能


MIDI_CREATE_DEFAULT_INSTANCE();  //启用MIDI库

void setup() {
  // Serial.begin(31250);  //midi协议的波特率就是31.25k 所以这里也要使用否则乱码

  // pinMode(LDAC, OUTPUT);               //DAC trans
  // pinMode(SS, OUTPUT);                 //DAC trans
  pinMode(CLOCK_PIN, OUTPUT);  //CLK_OUT
  pinMode(2, OUTPUT);          //Gate
  pinMode(3, OUTPUT);          //Gate
  pinMode(4, OUTPUT);          //Gate
  pinMode(5, OUTPUT);          //Gate
  pinMode(6, OUTPUT);          //Gate
  pinMode(7, OUTPUT);          //Gate
  pinMode(8, OUTPUT);          //Gate

  pinMode(CONFIG1_PIN, INPUT_PULLUP);  //config
  pinMode(CONFIG2_PIN, INPUT_PULLUP);  //config

  MIDI.begin(MIDI_CHANNEL_OMNI);  // MIDI CH ALL Listen

  // if (digitalRead(CONFIG1_PIN) == 0) ch1 = 1;      //D10插上
  // if (digitalRead(CONFIG1_PIN) == 1) ch1 = 1;      //D10取下
  if (digitalRead(CONFIG2_PIN) == 0) cc_mode = 1;  //D12插上
  if (digitalRead(CONFIG2_PIN) == 1) cc_mode = 0;  //D12取下

  for (int i = 2; i <= 8; i++)
    digitalWrite(i, HIGH);
  digitalWrite(CLOCK_PIN, HIGH);
  delay(3000);
  for (int i = 2; i <= 8; i++)
    digitalWrite(i, LOW);
  digitalWrite(CLOCK_PIN, LOW);
}

void loop() {
  // Serial.println(" ");

  controlChange();  //midi operation

  if (cc_mode == 0)  //D12取下 单通道多音符模式
    singleChannel();
  if (cc_mode == 1)  //D12插上 多通道但音符模式
    multChannel();

  timerLoop();
}

void controlChange() {
  if (MIDI.read()) {
    enable_rand_trig = 0;        //随机触发功能: 每当收到midi信号时 就禁用随机触发功能
    pinMode(CLOCK_PIN, OUTPUT);  //随机触发功能: 每当收到midi信号时 恢复时钟接口为输出

    switch (MIDI.getType()) {
      case midi::Clock:
        if (clock_count == 0) {
          digitalWrite(CLOCK_PIN, HIGH);
        }
        if (clock_count != 0) {
          digitalWrite(CLOCK_PIN, LOW);
        }
        clock_count++;
        if (clock_count >= clock_max) {
          clock_count = 0;
        }
        break;
      case midi::Start:
        clock_count = 0;
        break;
      case midi::AfterTouchPoly:
        // if (cc_mode == 0) OUT_PWM(CV3_PIN, MIDI.getData2());  //3个cv映射输出力度cv
        break;
      case midi::PitchBend:
        break;
      case midi::AllNotesOff:
        for (int i = 2; i <= 8; i++)
          digitalWrite(i, LOW);
        break;
      case midi::Stop:
        clock_count = 0;
        for (int i = 2; i <= 8; i++)
          digitalWrite(i, LOW);
        break;
      case midi::ControlChange:
        switch (MIDI.getData1()) {
          case 10:  //切换四种模式 //change cc maping in modular
            cc_mode = MIDI.getData2() >> 5;
            break;
          case 1:  //输出mod转化的CV
            break;
          case 21:  //seq pitch
            break;
          case 22:  //gate pitch
            break;
          case 23:  //vel pitch
            break;
          case 24:  //切换时钟div //clock_rate setting
            clock_rate = MIDI.getData2() >> 5;
            clock_max = 24 * clock_div / clock_rate;
            break;
          case 25:  //调整seq length //length范围:1-16
            break;
          case 26:  //page 预留
            break;
          case 27:  //调整loop mode //范围0-3
            break;
          case 28:  //调整播放状态 //范围0-3
            break;
        }
        break;  //ControlChange
    }
  }
}

void singleChannel() {
  if (MIDI.getChannel() == ch1) {  //MIDI CH1
    switch (MIDI.getType()) {
      case midi::NoteOn:                      //if NoteOn c1是第一个音符
        int note_tmp = MIDI.getData1() - 24;  //note number
        if (note_tmp < 9)
          digitalWrite(note_tmp, HIGH);  //Gate》HIGH
        break;
      case midi::NoteOff:
        int note_tmp2 = MIDI.getData1() - 24;  //note number
        if (note_tmp < 9)
          digitalWrite(note_tmp2, LOW);  //Gate》LOW
        break;
    }
  }
}

void multChannel() {
  switch (MIDI.getType()) {
    case midi::NoteOn:                            //if NoteOn c1是第一个音符
      digitalWrite(MIDI.getChannel() + 1, HIGH);  //Gate》HIGH
      break;
    case midi::NoteOff:
      digitalWrite(MIDI.getChannel() + 1, LOW);  //Gate》LOW
      break;
  }
  // switch (MIDI.getChannel()) {
  //   case 1:
  //     break;
  //   case 2:
  //     break;
  //   case 3:
  //     break;
  //   case 4:
  //     break;
  //   case 5:
  //     break;
  //   case 6:
  //     break;
  //   case 7:
  //     break;
  // }
}

unsigned long timer_start_time = 0;  // 用于记录事件开始时间
void timerLoop() {
  if (millis() - timer_start_time >= 10000) {  //事件持续10秒钟或以上
                                               //在这里编写你的代码来响应这个事件
    enable_rand_trig = 1;
    triggerListener();
  }
}

void timerReset() {
  timer_start_time = millis();  // 重置开始时间
}
