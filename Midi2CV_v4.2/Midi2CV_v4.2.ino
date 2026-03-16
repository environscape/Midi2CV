#include <MIDI.h>
#include <SPI.h>  //DAC通信用

#define CONFIG1_PIN 8      //配置1
#define CONFIG2_PIN 12     //配置2
#define CLOCK_PIN 2        //CLK
#define GATE1_PIN 4        //Gate1
#define GATE2_PIN 7        //Gate2
#define CV1_PIN 3          //CV1
#define CV2_PIN 5          //CV2
#define CV3_PIN 6          //CV3
#define VER1_PIN A0        //版本判断引脚
#define VER1_TUNER_PIN A1  //调谐引脚
#define OCT_CONST 68.25    //V/OCT 常量

byte midi_mode = 0;         //用于更改当前cc映射模式
byte enable_rand_trig = 0;  //概率触发模式启用状态 0不启用 1启用

#include "io.h"
#include "rnd_mode.h"

unsigned long total_clock = 0;  // 总计数器：累计收到的所有MIDI Clock事件数
byte clock_count = 0;           //clock计数器
byte clock_max = 6;             //clock分辨率
int clock_rate = 0;             //Clock速率

byte bend_range = 0;
byte bend_msb = 0;
byte bend_lsb = 0;
int after_bend_pitch = 0;

int note_no1 = 0;
int note_no2 = 0;
byte note_on_count1 = 0;  //当多个音符打开且其中一个音符关闭时，最后一个音符不消失。
byte note_on_count2 = 0;  //当多个音符打开且其中一个音符关闭时，最后一个音符不消失。
byte tmp_last_note1 = -1;
byte tmp_last_note2 = -1;



MIDI_CREATE_DEFAULT_INSTANCE();  //启用MIDI库

void setup() {
  Serial.begin(31250);  //midi协议的波特率就是31.25k 所以这里也要使用否则乱码

  pinMode(LDAC, OUTPUT);               //DAC trans
  pinMode(SS, OUTPUT);                 //DAC trans
  pinMode(CLOCK_PIN, OUTPUT);          //CLK_OUT
  pinMode(GATE1_PIN, OUTPUT);          //CLK_OUT
  pinMode(GATE2_PIN, OUTPUT);          //CLK_OUT
  pinMode(CV1_PIN, OUTPUT);            //CV_OUT
  pinMode(CV2_PIN, OUTPUT);            //CV_OUT
  pinMode(CV3_PIN, OUTPUT);            //CV_OUT
  pinMode(CONFIG1_PIN, INPUT_PULLUP);  //config
  pinMode(CONFIG2_PIN, INPUT_PULLUP);  //config
  pinMode(VER1_PIN, INPUT_PULLUP);     //pcb version>=4.0
  pinMode(VER1_TUNER_PIN, INPUT);      //读取调谐情况

  MIDI.begin(MIDI_CHANNEL_OMNI);  // MIDI CH ALL Listen

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);            // bit order
  SPI.setClockDivider(SPI_CLOCK_DIV4);  // クロック(CLK)をシステムクロックの1/4で使用(16MHz/4)
  SPI.setDataMode(SPI_MODE0);           // クロック極性０(LOW)　クロック位相０

  midi_mode = (1 - digitalRead(CONFIG1_PIN)) + (1 - digitalRead(CONFIG2_PIN)) * 2;  //该模式相当于以下四种情况
  // if (digitalRead(CONFIG1_PIN) == 1 && digitalRead(CONFIG2_PIN) == 1) midi_mode = 0;  //默认模式 什么都不插
  // if (digitalRead(CONFIG1_PIN) == 0 && digitalRead(CONFIG2_PIN) == 1) midi_mode = 1;  //读取d8d12跳线帽 默认1 插上0
  // if (digitalRead(CONFIG1_PIN) == 1 && digitalRead(CONFIG2_PIN) == 0) midi_mode = 2;  //读取d8d12跳线帽 默认1 插上0
  // if (digitalRead(CONFIG1_PIN) == 0 && digitalRead(CONFIG2_PIN) == 0) midi_mode = 3;  //读取d8d12跳线帽 默认1 插上0

  digitalWrite(CLOCK_PIN, HIGH);
  digitalWrite(GATE1_PIN, HIGH);
  digitalWrite(GATE2_PIN, HIGH);
  delay(3000);
  digitalWrite(CLOCK_PIN, LOW);  //开机启动 三秒led显示
  digitalWrite(GATE1_PIN, LOW);  //开机启动 三秒led显示
  digitalWrite(GATE2_PIN, LOW);  //开机启动 三秒led显示
}

void loop() {
  // Serial.println("loop");
  if (MIDI.read()) {
    enable_rand_trig = 0;  //随机触发功能: 每当收到midi信号时 就禁用随机触发功能
    controlChange();       //midi cc
    if (midi_mode == 0 || midi_mode == 1) {
      firstVoct();   //midi ch1
      secondVoct();  //midi ch2
    }
    if (midi_mode == 2) {
      multCHGate();
    }
    if (midi_mode == 3) {
      singleCHGate();
    }
  }

  timerLoop();  //计时器循环监听
}

void controlChange() {
  setFastPWM();                //pwm频率约为62.5khz
  pinMode(CLOCK_PIN, OUTPUT);  //随机触发功能: 每当收到midi信号时 恢复时钟接口为输出
  timerReset();                //计时器重置
  // if (MIDI.getChannel()) {
  switch (MIDI.getType()) {
    case midi::Clock:
      clock_count = total_clock % clock_max;  // 步骤1：先基于当前total_clock计算clock_count（第一次时total_clock=0）
      if (clock_count == 0) {                 // 步骤2：根据clock_count触发电平（第一次时clock_count=0，输出高电平）
        digitalWrite(CLOCK_PIN, HIGH);
      } else {
        digitalWrite(CLOCK_PIN, LOW);
      }
      total_clock++;  // 步骤3：最后累加总计数器（确保下次计算用更新后的值）
      break;
    case midi::ControlChange:
      switch (MIDI.getData1()) {
        case 24:                                 // 切换时钟div //clock_rate setting
          byte div_zone = MIDI.getData2() / 16;  // 结果：0~7（对应8个区间）
          switch (div_zone) {                    // 步骤2：按区间映射到目标clock_max（触发阈值）
            case 0: clock_max = 72; break;       // CC24 0-15 → 1/3次/四分音符
            case 1: clock_max = 48; break;       // CC24 16-31 → 1/2次/四分音符
            case 2: clock_max = 24; break;       // CC24 32-47 → 1次/四分音符
            case 3: clock_max = 12; break;       // CC24 48-63 → 2次/四分音符
            case 4: clock_max = 8; break;        // CC24 64-79 → 3次/四分音符
            case 5: clock_max = 6; break;        // CC24 80-95 → 4次/四分音符
            case 6: clock_max = 4; break;        // CC24 96-111 → 6次/四分音符
            case 7: clock_max = 3; break;        // CC24 112-127 → 8次/四分音符
          }
          if (clock_max < 1) clock_max = 1;  // 步骤3：安全兜底（避免极端值导致clock_max=0，仅防御性判断）
          break;
        case 1:                                                  //输出mod转化的CV
          if (midi_mode < 2) OUT_PWM(CV3_PIN, MIDI.getData2());  //gate模式不做mod触发
          break;
        case 10:  //切换四种模式 //change cc maping in modular
          midi_mode = MIDI.getData2() >> 5;
          break;
      }
      break;  //ControlChange
    case midi::AllNotesOff:
      note_on_count1 = 0;
      digitalWrite(GATE1_PIN, LOW);  //Gate》LOW
      note_on_count2 = 0;
      digitalWrite(GATE2_PIN, LOW);  //Gate》LOW
      break;
    case midi::Start:
      clock_count = 0;
      total_clock = 0;  // 同步重置当前计数
      break;
    case midi::Stop:
      note_on_count1 = 0;
      note_on_count2 = 0;
      tmp_last_note1 = -1;
      tmp_last_note2 = -1;
      clock_count = 0;
      digitalWrite(GATE1_PIN, LOW);  //Gate》LOW
      digitalWrite(GATE2_PIN, LOW);  //Gate》LOW
      break;
    case midi::AfterTouchPoly:
      // if (midi_mode == 0) OUT_PWM(CV3_PIN, MIDI.getData2());  //3个cv映射输出力度cv
      break;
    case midi::PitchBend:
      bend_lsb = MIDI.getData1();  //LSB
      bend_msb = MIDI.getData2();  //MSB
      bend_range = bend_msb;       //0 to 127
      if (bend_range > 64) {
        after_bend_pitch = OCT_CONST * note_no1 + OCT_CONST * note_no1 * (bend_range - 64) * 4 / 10000;
        OUT_VOCT1(after_bend_pitch);
        after_bend_pitch = OCT_CONST * note_no1 + OCT_CONST * note_no2 * (bend_range - 64) * 4 / 10000;
        OUT_VOCT2(after_bend_pitch);
      } else if (bend_range < 64) {
        after_bend_pitch = OCT_CONST * note_no1 - OCT_CONST * note_no1 * (64 - bend_range) * 4 / 10000;
        OUT_VOCT1(after_bend_pitch);
        after_bend_pitch = OCT_CONST * note_no1 - OCT_CONST * note_no2 * (64 - bend_range) * 4 / 10000;
        OUT_VOCT2(after_bend_pitch);
      }
      break;
  }
}

//mode=0/1时的第一个通道
void firstVoct() {
  if (MIDI.getChannel() == (2 * midi_mode + 1)) {  //MIDI CH1
    switch (MIDI.getType()) {
      case midi::NoteOn:  //if NoteOn
        note_on_count1++;
        note_no1 = MIDI.getData1() - 24;  //note number
        tmp_last_note1 = MIDI.getData1();
        if (note_no1 < 0) {
          note_no1 = 0;
        } else if (note_no1 >= 61) {
          note_no1 = 60;
        }
        OUT_VOCT1(OCT_CONST * note_no1);    //V/OCT LSB for DAC》参照          // OUT_VOCT1(V_OCT[note_no1]);
        OUT_PWM(CV1_PIN, MIDI.getData2());  //3个cv映射输出力度cv
        digitalWrite(GATE1_PIN, HIGH);      //Gate》HIGH
        break;
      case midi::NoteOff:
        if (tmp_last_note1 == MIDI.getData1())
          digitalWrite(GATE1_PIN, LOW);  //Gate 》LOW
        break;
    }
  }  //MIDI CH1
}

//mode=0/1时的第二个通道
void secondVoct() {
  if (MIDI.getChannel() == (2 * midi_mode + 2)) {  //MIDI CH2
    switch (MIDI.getType()) {
      case midi::NoteOn:  //if NoteOn
        note_on_count2++;
        note_no2 = MIDI.getData1() - 24;  //note number
        tmp_last_note2 = MIDI.getData1();
        if (note_no2 < 0) {
          note_no2 = 0;
        } else if (note_no2 >= 61) {
          note_no2 = 60;
        }
        OUT_VOCT2(OCT_CONST * note_no2);    //V/OCT LSB for DAC》参照
        OUT_PWM(CV2_PIN, MIDI.getData2());  //3个cv映射输出力度cv
        digitalWrite(GATE2_PIN, HIGH);      //Gate》HIGH
        break;
      case midi::NoteOff:  //if NoteOff 关闭后
        if (tmp_last_note2 == MIDI.getData1())
          digitalWrite(GATE2_PIN, LOW);  //Gate 》LOW
        break;
    }  //MIDI CH2
  }
}

//mode=2时 多通道每个通道触发gate ch为1-7
void multCHGate() {
  switch (MIDI.getType()) {
    case midi::NoteOn:  //if NoteOn c1是第一个音符
      switch (MIDI.getChannel()) {
        case 1:
          OUT_VOCT1(4095);
          break;
        case 2:
          digitalWrite(GATE1_PIN, 1);
          break;
        case 3:
          OUT_PWM(CV1_PIN, 127);
          break;
        case 4:
          OUT_VOCT2(4095);
          break;
        case 5:
          digitalWrite(GATE2_PIN, 1);
          break;
        case 6:
          OUT_PWM(CV2_PIN, 127);
          break;
        case 7:
          OUT_PWM(CV3_PIN, 127);
          break;
      }
      break;
    case midi::NoteOff:
      switch (MIDI.getChannel()) {
        case 1:
          OUT_VOCT1(0);
          break;
        case 2:
          digitalWrite(GATE1_PIN, 0);
          break;
        case 3:
          OUT_PWM(CV1_PIN, 0);
          break;
        case 4:
          OUT_VOCT2(0);
          break;
        case 5:
          digitalWrite(GATE2_PIN, 0);
          break;
        case 6:
          OUT_PWM(CV2_PIN, 0);
          break;
        case 7:
          OUT_PWM(CV3_PIN, 0);
          break;
      }
      break;
  }
}

//mode=3时 ch10单通道多音符触发gate
void singleCHGate() {
  if (MIDI.getChannel() == 10) {     //仅监听MIDI CH10
    int note_num = MIDI.getData1();  //获取MIDI音符编号
    int note_mod = note_num % 12;    //计算音符模12（判断音名：C/D/E/F/G/A/B）
    switch (MIDI.getType()) {
      case midi::NoteOn:     //音符开启逻辑
        switch (note_mod) {  //仅处理无升降号的自然音，升降号音符直接跳过
          case 0:            //C音（所有八度的C）→ 对应multCHGate case1
            OUT_VOCT1(4095);
            break;
          case 2:  //D音 → 对应multCHGate case2
            digitalWrite(GATE1_PIN, HIGH);
            break;
          case 4:  //E音 → 对应multCHGate case3
            OUT_PWM(CV1_PIN, 127);
            break;
          case 5:  //F音 → 对应multCHGate case4
            OUT_VOCT2(4095);
            break;
          case 7:  //G音 → 对应multCHGate case5
            digitalWrite(GATE2_PIN, HIGH);
            break;
          case 9:  //A音 → 对应multCHGate case6
            OUT_PWM(CV2_PIN, 127);
            break;
          case 11:  //B音 → 对应multCHGate case7
            OUT_PWM(CV3_PIN, 127);
            break;
        }
        break;
      case midi::NoteOff:  //音符关闭逻辑
        switch (note_mod) {
          case 0:  //C音关闭 → 对应multCHGate case1关闭
            OUT_VOCT1(0);
            break;
          case 2:  //D音关闭 → 对应multCHGate case2关闭
            digitalWrite(GATE1_PIN, LOW);
            break;
          case 4:  //E音关闭 → 对应multCHGate case3关闭
            OUT_PWM(CV1_PIN, 0);
            break;
          case 5:  //F音关闭 → 对应multCHGate case4关闭
            OUT_VOCT2(0);
            break;
          case 7:  //G音关闭 → 对应multCHGate case5关闭
            digitalWrite(GATE2_PIN, LOW);
            break;
          case 9:  //A音关闭 → 对应multCHGate case6关闭
            OUT_PWM(CV2_PIN, 0);
            break;
          case 11:  //B音关闭 → 对应multCHGate case7关闭
            OUT_PWM(CV3_PIN, 0);
            break;
        }
        break;
    }
  }
}
