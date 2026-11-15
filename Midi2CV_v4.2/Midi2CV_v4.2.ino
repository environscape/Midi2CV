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

byte midi_mode = 0;  //用于更改当前cc映射模式

#include "io.h"
#include "rnd_mode.h"

unsigned long total_clock = 0;  // 总计数器：累计收到的所有MIDI Clock事件数
byte clock_count = 0;           //clock计数器
byte clock_max = 6;             //clock分辨率
int clock_rate = 0;             //Clock速率
bool clock_output_enabled = 1;  // 时钟输出启用标志：默认启用，STOP时关闭，START时重新打开

byte bend_msb = 0;
byte bend_lsb = 0;
int after_bend_pitch = 0;

int note_no1 = 0;
int note_no2 = 0;
int poly_on1 = 0;             //mode=1复音第一声部音高
int poly_on2 = 0;             //mode=1复音第二声部音高
int poly_note1 = -1;          //mode=1声部1当前音符，-1为空闲
int poly_note2 = -1;          //mode=1声部2当前音符，-1为空闲
unsigned long poly_seq1 = 0;  //声部1分配顺序，用于抢占最旧声部
unsigned long poly_seq2 = 0;  //声部2分配顺序
unsigned long poly_tick = 0;  //分配顺序计数器
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
    if (midi_mode == 0) {
      firstVoct();   //midi ch1
      secondVoct();  //midi ch2
    }
    if (midi_mode == 1) {
      dualPoly();  //midi ch1 双复音
    }
    if (midi_mode == 2) {
      multCHGate();
    }
    if (midi_mode == 3) {
      singleCHGate();
    }
    controlChange();  //midi cc
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
      if (clock_output_enabled) {             // 步骤2：时钟输出启用时才操作CLOCK_PIN
        if (clock_count == 0) {               // 步骤3：根据clock_count触发电平（第一次时clock_count=0，输出高电平）
          digitalWrite(CLOCK_PIN, HIGH);
        } else {
          digitalWrite(CLOCK_PIN, LOW);
        }
      }
      total_clock++;  // 步骤4：最后累加总计数器（确保下次计算用更新后的值）
      break;
    case midi::ControlChange:
      switch (MIDI.getData1()) {
        case 24:  // 切换时钟div //clock_rate setting
          {
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
          }
          break;
        case 1:                                                  //输出mod转化的CV
          if (midi_mode < 2) OUT_PWM(CV3_PIN, MIDI.getData2());  //gate模式不做mod触发
          break;
        case 10:  //切换四种模式 //change cc maping in modular
          midi_mode = MIDI.getData2() >> 5;
          resetPoly();  //切换模式时清空复音状态并关闭Gate
          break;
      }
      break;  //ControlChange
    case midi::AllNotesOff:
      resetPoly();
      break;
    case midi::Start:
      clock_output_enabled = 1;  // 重新启用时钟输出
      clock_count = 0;
      total_clock = 0;  // 同步重置当前计数
      break;
    case midi::Stop:
      clock_output_enabled = 0;  // 禁用时钟输出
      digitalWrite(CLOCK_PIN, LOW);  // 停止时钟信号，留给用户手动归零
      resetPoly();
      tmp_last_note1 = -1;
      tmp_last_note2 = -1;
      clock_count = 0;
      break;
    case midi::AfterTouchPoly:
      // if (midi_mode == 0) OUT_PWM(CV3_PIN, MIDI.getData2());  //3个cv映射输出力度cv
      break;
    case midi::PitchBend: {
      bend_lsb = MIDI.getData1();                  //LSB
      bend_msb = MIDI.getData2();                  //MSB
      int bend_value = (bend_msb << 7) | bend_lsb;  //14bit：0~16383，中点8192
      //基准音高：复音模式用各声部音高，其余模式用双通道音高，两路各自独立
      long base1 = OCT_CONST * ((midi_mode == 1) ? poly_on1 : note_no1);
      long base2 = OCT_CONST * ((midi_mode == 1) ? poly_on2 : note_no2);
      //偏移比例与原7bit公式一致；回中(bend_value=8192)时恢复基准音高
      after_bend_pitch = base1 + base1 * (long)(bend_value - 8192) / 320000;
      OUT_VOCT1(after_bend_pitch);
      after_bend_pitch = base2 + base2 * (long)(bend_value - 8192) / 320000;
      OUT_VOCT2(after_bend_pitch);
      break;
    }
  }
}

//mode=0时的第一个通道
void firstVoct() {
  if (MIDI.getChannel() == (2 * midi_mode + 1)) {  //MIDI CH1
    switch (MIDI.getType()) {
      case midi::NoteOn:  //if NoteOn
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

//mode=0时的第二个通道
void secondVoct() {
  if (MIDI.getChannel() == (2 * midi_mode + 2)) {  //MIDI CH2
    switch (MIDI.getType()) {
      case midi::NoteOn:  //if NoteOn
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

//mode=1复音状态复位：关闭两路Gate并清空声部
void resetPoly() {
  poly_note1 = -1;
  poly_note2 = -1;
  digitalWrite(GATE1_PIN, LOW);
  digitalWrite(GATE2_PIN, LOW);
}

//mode=1 向指定声部分配音符；声部已在发声时先拉低Gate再立即拉高，重新触发包络
void polyAssign(byte voice, int note, byte velocity) {
  int pitch = note - 24;  //note number
  if (pitch < 0) {
    pitch = 0;
  } else if (pitch >= 61) {
    pitch = 60;
  }
  if (voice == 1) {
    bool was_active = (poly_note1 != -1);
    poly_note1 = note;
    poly_on1 = pitch;
    poly_seq1 = poly_tick++;
    OUT_VOCT1(OCT_CONST * poly_on1);  //V/OCT LSB for DAC》参照
    OUT_PWM(CV1_PIN, velocity);       //第一声部力度输出到CV1
    if (was_active) {
      digitalWrite(GATE1_PIN, LOW);
    }
    digitalWrite(GATE1_PIN, HIGH);  //Gate》HIGH
  } else {
    bool was_active = (poly_note2 != -1);
    poly_note2 = note;
    poly_on2 = pitch;
    poly_seq2 = poly_tick++;
    OUT_VOCT2(OCT_CONST * poly_on2);  //V/OCT LSB for DAC》参照
    OUT_PWM(CV2_PIN, velocity);       //第二声部力度输出到CV2
    if (was_active) {
      digitalWrite(GATE2_PIN, LOW);
    }
    digitalWrite(GATE2_PIN, HIGH);  //Gate》HIGH
  }
}

//mode=1时 CH1单通道双复音：按音符追踪声部，两声部占满时抢占最旧声部
void dualPoly() {
  if (MIDI.getChannel() != 1) return;  //仅监听MIDI CH1
  int note = MIDI.getData1();
  switch (MIDI.getType()) {
    case midi::NoteOn:
      if (note == poly_note1) {  //同音重复：刷新该声部并重新触发
        polyAssign(1, note, MIDI.getData2());
      } else if (note == poly_note2) {
        polyAssign(2, note, MIDI.getData2());
      } else if (poly_note1 == -1) {  //优先空闲声部
        polyAssign(1, note, MIDI.getData2());
      } else if (poly_note2 == -1) {
        polyAssign(2, note, MIDI.getData2());
      } else if (poly_seq1 <= poly_seq2) {  //占满时抢占最早分配的声部
        polyAssign(1, note, MIDI.getData2());
      } else {
        polyAssign(2, note, MIDI.getData2());
      }
      break;
    case midi::NoteOff:  //只释放音符匹配的声部，与按下/松开顺序无关
      if (note == poly_note1) {
        poly_note1 = -1;
        digitalWrite(GATE1_PIN, LOW);  //Gate》LOW
      } else if (note == poly_note2) {
        poly_note2 = -1;
        digitalWrite(GATE2_PIN, LOW);  //Gate》LOW
      }
      break;
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
  if (MIDI.getChannel() == 1) {      //仅监听MIDI CH10
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
