#include <MIDI.h>
#include <SPI.h>  //DAC通信用

#define CONFIG1_PIN 8    //配置1
#define CONFIG2_PIN 12   //配置2
#define CLOCK_PIN 2      //CLK
#define GATE1_PIN 4      //Gate1
#define GATE2_PIN 7      //Gate2
#define CV1_PIN 3        //CV1
#define CV2_PIN 5        //CV2
#define CV3_PIN 6        //CV3
#define OCT_CONST 68.25  //V/OCT 常量

#include "output.h"
#include "inner_sequencer.h"
#include "random_trig.h"
#include "fast_pwm.h"

byte ch1 = 1;
byte ch2 = 2;

byte bend_range = 0;
byte bend_msb = 0;
byte bend_lsb = 0;
int after_bend_pitch = 0;

int note_no1 = 0;  //noteNo=21(A0)～60(A5) total 61,マイナスの値を取るのでint 因为取负值，所以int
int note_no2 = 0;  //noteNo=21(A0)～60(A5) total 61,マイナスの値を取るのでint 因为取负值，所以int
int poly_on1 = 0;  //noteNo=21(A0)～60(A5) total 61,マイナスの値を取るのでint 因为取负值，所以int
int poly_on2 = 0;  //noteNo=21(A0)～60(A5) total 61,マイナスの値を取るのでint 因为取负值，所以int

byte note_on_count1 = 0;  //当多个音符打开且其中一个音符关闭时，最后一个音符不消失。
byte note_on_count2 = 0;  //当多个音符打开且其中一个音符关闭时，最后一个音符不消失。
byte poly_on_count = 0;   //当多个音符打开且其中一个音符关闭时，最后一个音符不消失。
byte tmp_last_note1 = -1;
byte tmp_last_note2 = -1;

byte clock_count = 0;  //clock计数器
byte clock_max = 24;   //clock分辨率
int clock_rate = 0;    //Clock速率
int clock_div = 1;     //Clock div 特殊用途

byte cc_mode = 0;           //用于更改当前cc映射模式
byte enable_rand_trig = 0;  // 0不启用 1启用

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

  MIDI.begin(MIDI_CHANNEL_OMNI);  // MIDI CH ALL Listen

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);            // bit order
  SPI.setClockDivider(SPI_CLOCK_DIV4);  // クロック(CLK)をシステムクロックの1/4で使用(16MHz/4)
  SPI.setDataMode(SPI_MODE0);           // クロック極性０(LOW)　クロック位相０

  if (digitalRead(CONFIG1_PIN) == 0) cc_mode = 1;  //读取d8跳线帽 默认1 插上0 如果插上给则默认进入
  if (digitalRead(CONFIG2_PIN) == 0) cc_mode = 2;  //读取d12跳线帽 默认1 插上0 如果插上给则默认进入
  // cc_mode = 2;

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

  controlChange();  //midi cc
  firstChannel();   //midi ch1
  secondChannel();  //midi ch2
  timerLoop();      //计时器循环监听
}

void controlChange() {
  if (MIDI.read()) {
    setFastPWM();                //pwm频率约为62.5khz
    enable_rand_trig = 0;        //随机触发功能: 每当收到midi信号时 就禁用随机触发功能
    pinMode(CLOCK_PIN, OUTPUT);  //随机触发功能: 每当收到midi信号时 恢复时钟接口为输出
    timerReset();                //计时器重置
    // if (MIDI.getChannel()) {
    switch (MIDI.getType()) {
      case midi::Clock:
        if (clock_count == 0) {
          digitalWrite(CLOCK_PIN, HIGH);
          //  if (cc_mode == 2) sequencerNext();       //音序器执行下一步
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
        bend_lsb = MIDI.getData1();  //LSB
        bend_msb = MIDI.getData2();  //MSB
        bend_range = bend_msb;       //0 to 127

        if (cc_mode != 1) {  //常规通道模式
          if (bend_range > 64) {
            after_bend_pitch = OCT_CONST * note_no1 + OCT_CONST * note_no1 * (bend_range - 64) * 4 / 10000;
            OUT_CV1(after_bend_pitch);
            after_bend_pitch = OCT_CONST * note_no1 + OCT_CONST * note_no2 * (bend_range - 64) * 4 / 10000;
            OUT_CV2(after_bend_pitch);
          } else if (bend_range < 64) {
            after_bend_pitch = OCT_CONST * note_no1 - OCT_CONST * note_no1 * (64 - bend_range) * 4 / 10000;
            OUT_CV1(after_bend_pitch);
            after_bend_pitch = OCT_CONST * note_no1 - OCT_CONST * note_no2 * (64 - bend_range) * 4 / 10000;
            OUT_CV2(after_bend_pitch);
          }
        } else {
          if (bend_range > 64) {
            after_bend_pitch = OCT_CONST * poly_on1 + OCT_CONST * poly_on1 * (bend_range - 64) * 4 / 10000;
            OUT_CV1(after_bend_pitch);
            after_bend_pitch = OCT_CONST * poly_on1 + OCT_CONST * poly_on2 * (bend_range - 64) * 4 / 10000;
            OUT_CV2(after_bend_pitch);
          } else if (bend_range < 64) {
            after_bend_pitch = OCT_CONST * poly_on1 - OCT_CONST * poly_on1 * (64 - bend_range) * 4 / 10000;
            OUT_CV1(after_bend_pitch);
            after_bend_pitch = OCT_CONST * poly_on1 - OCT_CONST * poly_on2 * (64 - bend_range) * 4 / 10000;
            OUT_CV2(after_bend_pitch);
          }
        }
        break;
      case midi::AllNotesOff:
        note_on_count1 = 0;
        digitalWrite(GATE1_PIN, LOW);  //Gate》LOW
        note_on_count2 = 0;
        digitalWrite(GATE2_PIN, LOW);  //Gate》LOW
        poly_on_count = 0;
        break;
      case midi::Stop:
        note_on_count1 = 0;
        note_on_count2 = 0;
        tmp_last_note1 = -1;
        tmp_last_note2 = -1;
        poly_on_count = 0;

        clock_count = 0;
        digitalWrite(GATE1_PIN, LOW);  //Gate》LOW
        digitalWrite(GATE2_PIN, LOW);  //Gate》LOW
        break;
      case midi::ControlChange:
        switch (MIDI.getData1()) {
          case 10:  //切换四种模式 //change cc maping in modular
            cc_mode = MIDI.getData2() >> 5;
            if (cc_mode == 0) {  //1/2通道模式
              ch1 = 1;
              ch2 = 2;
            }
            if (cc_mode == 1) {  //3/4通道模式
              ch1 = 3;
              ch2 = 4;
              clock_div = 2;
            }
            if (cc_mode == 2) {  //Seq模式
              ch1 = 1;
              ch2 = 2;
            }
            if (cc_mode == 3) {  //复音模式
              ch1 = 1;
              ch2 = 2;
            }
            break;
          case 1:  //输出mod转化的CV
            OUT_PWM(CV3_PIN, MIDI.getData2());
            break;
          case 21:  //seq pitch
            seq_pitch[seq_select] = MIDI.getData2() >> 1;
            if (seq_pitch[seq_select] > 60) seq_pitch[seq_select] = 60;
            break;
          case 22:  //gate pitch
            seq_gate[seq_select] = MIDI.getData2() >> 1;
            break;
          case 23:  //vel pitch
            seq_vel[seq_select] = MIDI.getData2();
            break;
          case 24:  //切换时钟div //clock_rate setting
            clock_rate = MIDI.getData2() >> 5;
            clock_max = 24 * clock_div / clock_rate;  // 范围0-3

            // byte rate_temp = MIDI.getData2() / 16;      // 1. 将MIDI.getData2()(0-127)均匀映射到0~7（共8档），避免浪费范围   // 127/16≈7.93，取整后0~7
            // clock_rate = rate_temp + 1;                 // 2. 强制clock_rate≥1，规避除法为0的错误，最终范围1~8
            // clock_max = (24 * clock_div) / clock_rate;  // 3. 重新计算clock_max，此时clock_rate≥1，无除法错误；clock_rate越大，clock_max越小，翻转越快
            // if (clock_max < 1) clock_max = 1;           // （可选）限制clock_max最小为1，避免极端情况下clock_max=0导致计数逻辑异常
            break;
            break;
          case 25:  //调整seq length //length范围:1-16
            seq_length = (MIDI.getData2() >> 3) + 1;
            break;
          case 26:  //page 预留
            seq_page = MIDI.getData2() >> 8;
            break;
          case 27:  //调整loop mode //范围0-3
            seq_loopmode = MIDI.getData2() >> 5;
            break;
          case 28:  //调整播放状态 //范围0-3
            seq_state = MIDI.getData2() >> 5;
            break;
        }
        if (100 < MIDI.getData1() && MIDI.getData1() < 117) {
          seq_select = MIDI.getData1() - 101;
        }
        break;  //ControlChange
    }
  }
}

void firstChannel() {
  if (MIDI.getChannel() == ch1) {  //MIDI CH1
                                   // if (MIDI.read(ch1)) {  //MIDI CH1
    switch (MIDI.getType()) {
      case midi::NoteOn:     //if NoteOn
        if (cc_mode != 1) {  //常规通道模式
          note_on_count1++;
          note_no1 = MIDI.getData1() - 24;  //note number
          tmp_last_note1 = MIDI.getData1();
          if (note_no1 < 0) {
            note_no1 = 0;
          } else if (note_no1 >= 61) {
            note_no1 = 60;
          }


          // OUT_CV1(V_OCT[note_no1]);
          OUT_CV1(OCT_CONST * note_no1);                            //V/OCT LSB for DAC》参照
          /*if (cc_mode == 0)*/ OUT_PWM(CV1_PIN, MIDI.getData2());  //3个cv映射输出力度cv
          digitalWrite(GATE1_PIN, HIGH);                            //Gate》HIGH
        } else {                                                    //复音模式
          poly_on_count++;
          if (poly_on_count == 1) {
            // if (poly_on_count % 2 == 1) {
            poly_on1 = MIDI.getData1() - 24;  //note number
            int velocity = MIDI.getData2();
            if (poly_on1 < 0) {
              poly_on1 = 0;
            } else if (poly_on1 >= 61) {
              poly_on1 = 60;
            }
            OUT_CV1(OCT_CONST * poly_on1);                 //V/OCT LSB for DAC》参照
            if (cc_mode != 3) OUT_PWM(CV2_PIN, velocity);  //3个cv映射输出力度cv
            digitalWrite(GATE1_PIN, HIGH);                 //Gate》HIGH
          }
          if (poly_on_count == 2) {
            // if (poly_on_count % 2 == 0) {
            poly_on2 = MIDI.getData1() - 24;  //note number
            int velocity = MIDI.getData2();
            if (poly_on2 < 0) {
              poly_on2 = 0;
            } else if (poly_on2 >= 61) {
              poly_on2 = 60;
            }
            OUT_CV2(OCT_CONST * poly_on2);           //V/OCT LSB for DAC》参照
            if (cc_mode == 0) OUT_PWM(5, velocity);  //3个cv映射输出力度cv
            digitalWrite(GATE2_PIN, HIGH);           //Gate》HIGH
          }
        }
        break;
      case midi::NoteOff:
        if (cc_mode != 1) {  //常规通道模式
          if (tmp_last_note1 == MIDI.getData1())
            digitalWrite(GATE1_PIN, LOW);  //Gate 》LOW
        } else {                           //双复音模式
          poly_on_count--;
          if (poly_on_count == 0) {
            // if (poly_on_count % 2 == 0) {
            digitalWrite(GATE1_PIN, LOW);  //Gate 》LOW
          }
          if (poly_on_count == 1) {
            // if (poly_on_count % 2 == 1) {
            digitalWrite(GATE2_PIN, LOW);  //Gate 》LOW
          }
        }
        break;
    }
  }
}

void secondChannel() {
  if (cc_mode == 2) return;        //如果是音序器模式 则不监听第二个通道的midi音符
  if (MIDI.getChannel() == ch2) {  //MIDI CH2
                                   // if (MIDI.read(ch2)) {  //MIDI CH1
    switch (MIDI.getType()) {
      case midi::NoteOn:     //if NoteOn
        if (cc_mode != 1) {  //常规通道模式
          note_on_count2++;
          note_no2 = MIDI.getData1() - 24;  //note number
          tmp_last_note2 = MIDI.getData1();
          if (note_no2 < 0) {
            note_no2 = 0;
          } else if (note_no2 >= 61) {
            note_no2 = 60;
          }
          // OUT_CV2(V_OCT[note_no2]);  //V/OCT LSB for DAC》参照
          OUT_CV2(OCT_CONST * note_no2);                        //V/OCT LSB for DAC》参照
          if (cc_mode == 0) OUT_PWM(CV2_PIN, MIDI.getData2());  //3个cv映射输出力度cv
          digitalWrite(GATE2_PIN, HIGH);                        //Gate》HIGH
        }
        break;
      case midi::NoteOff:  //if NoteOff 关闭后
        // if (note_on_count2 > 0) note_on_count2--;
        // if (note_on_count2 < 1) {
        if (tmp_last_note2 == MIDI.getData1())
          digitalWrite(GATE2_PIN, LOW);  //Gate 》LOW
        // }
        break;

    }  //MIDI CH2
  }
}

unsigned long timer_start_time = 0;  // 用于记录事件开始时间
void timerLoop() {
  if (millis() - timer_start_time >= 10000) {  //事件持续10秒钟或以上
    // Serial.println("random mode ");

    restoreDefaultPWM();  //恢复pwm
    enable_rand_trig = 1;
    triggerListener();
  }
}

void timerReset() {
  timer_start_time = millis();  // 重置开始时间
}
