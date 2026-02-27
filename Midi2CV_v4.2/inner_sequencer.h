/*
* 内部音序器实现
*/
byte seq_pitch[64] = { 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24 };
byte seq_gate[64] = { 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63 };
byte seq_vel[64] = { 0 };
byte seq_bpm = 0;       //音序器内部速度 暂不可用
byte seq_position = 0;  //音序器下标
byte seq_length = 8;    //音序器长度
byte seq_page = 0;      //音序器页码
byte seq_loopmode = 0;  //0:1:倒序2/3:随机
byte seq_state = 1;     //0:播放 1/2:暂停 3停止
byte seq_select = 1;    //当前选中的音序1-16

void sequencerView(int tmp_position) {  //音序器视图
  String view_str = "";
  view_str += " cc:" + String("cc_mode");
  view_str += " pos:" + String(tmp_position + 1);
  view_str += " rat:" + String("clock_rate");
  view_str += " len:" + String(seq_length);
  // view_str += " pag:" + String(seq_page);
  view_str += " mod:" + String(seq_loopmode);
  view_str += " ste:" + String(seq_state);
  view_str += " seq:";
  for (int i = 0; i < seq_length; i++) view_str += String(seq_pitch[i]) + " ";

  Serial.println(view_str);  //音序器视图发送
}


void sequencerNext() {  //音序器执行下一步

  int tmp_position = 0;    //播放模式下标计算
  switch (seq_loopmode) {  //当前播放模式下的播放顺序
    default:               //正序
      tmp_position = seq_position;
      break;
    case 1:  //倒序
      tmp_position = seq_length - seq_position - 1;
      break;
    case 2:  //随机1
      tmp_position = random(0, seq_length - 1);
      break;
    case 3:  //随机2 后一半随机播放
      tmp_position = random(seq_length / 2, seq_length - 1);
      break;
  }
  //gate随机功能判断
  byte tmp_gate = seq_gate[tmp_position] >> 3;  //得到0-7
  switch (tmp_gate) {
    default:  //1-6随机
      if (random(0, tmp_gate) > 3) digitalWrite(GATE2_PIN, HIGH);
      else digitalWrite(GATE2_PIN, LOW);
      break;
    case 0:  //低电平
      digitalWrite(GATE2_PIN, LOW);
      break;
    case 7:  //高电平
      digitalWrite(GATE2_PIN, HIGH);
      break;
  }
  // digitalWrite(GATE2_PIN, seq_gate[tmp_position] > 32 ? HIGH : LOW);  //GATE 大于32高电平 否则低电平
  OUT_CV2(OCT_CONST * seq_pitch[tmp_position]);  //V/OCT LSB for DAC》参照
  OUT_PWM(CV2_PIN, seq_vel[tmp_position]);       //VEL

  //播放状态控制
  if (seq_state == 0) {  // 播放
    seq_position++;
    if (seq_position >= seq_length) seq_position = 0;
  }
  if (seq_state == 3) {  // 停止
    seq_position = 0;
  }
  sequencerView(tmp_position);  //音序器视图
  //触发模式恢复触发
  digitalWrite(GATE2_PIN, LOW);  //TRIG 增加此行则表示触发
}

