# Midi2CV_Trs V4.1 使用手册

## 硬件及接口介绍:  

1xTrs Midi In(可通过跳线盘切换Trs TypeA/B)  
1xClock Out*(可通过cc24调整clock倍速**)  
2xNote Out(CV范围0-5V)  
2xGate Out(与音对应的门0-5V)  
3xCV Out(Midi CC信息转化而来的CV信号0-5V)  

*clock功能需要您的midi信号发出设备在设置中开启Clock Send(时钟同步发送)  否则将无法收到时钟讯号  
**clock sync功能需要您的midi信号发出设备在设置中开启Transport Send(走带控制发送) 否则无法根据播放同步时钟,时钟将自行周期触发  

## 硬件使用说明  
10p电源接口位于模块上方，接上电源时请注意红线方向与10p灰色排线保持持一致(虽然模块带有电源反接保护功能，但是依然要注意)。  
当模块接上电源线并供电时，三盏提示灯在开机时将会点亮三秒，用以提示用户模块供电正常。  
您可以查询您的Trs MIDI线的类型，模块出厂时，跳线帽都被设置成了Trs Type A类型。  
当模块接上Trs MIDI线之后，如果您的MIDI设备有开启时钟发送，模块的第一盏灯将会跟随MIDI设备的时钟闪动。这可以用于作为您的MIDI信号是否正常发送的充分判断依据。  
当MIDI设备有发送Channel1的音符，通常第二盏灯会随音符的note on亮起。第三盏灯也是同理。  

## 软件功能介绍:   
#### Midi设备可以通过CC10切换以下3种模式,如果10秒内没有MIDI信号输入,则进入概率模式  
|模式名称/接口名称|Clock|Note1|Gate1|Note2|Gate2|CV1|CV2|CV3|
|:-------------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|
|Midi:通道1&2模式|Clock|Ch1NotePitch|Ch1NoteOn|Ch2NotePitch|Ch2NoteOn|Vel1|Vel2|Mod|
|Midi:通道3&4模式|Clock1/2|Ch3NotePitch|Ch3NoteOn|Ch4NotePitch|Ch4NoteOn|Vel1|Vel2|Mod|
|Midi:复音模式|Clock|Ch1Poly1Pitch|Ch1Poly1On|Ch1Poly2Pitch|Ch1Poly2On|Vel1|Vel2|Mod|   
|概率模式***|Trig In|75%RandTrig|50%RandGate|25%RandGate|GateRandLength|RandVoltage|RandVoltageInv|RandVoltage1/2|

***如果任意时刻收到MIDI信号则即刻回到MIDI模式  
敬请期待更多功能更新

## 固件更新说明  
1.请安装arduino IDE  
2.请安装midi依赖库: https://github.com/FortySevenEffects/arduino_midi_library 下载zip 并用arduino IDE的库管理器安装该zip  
3.下载当前Midi2CV固件程序,打开项目中对应的.ino 文件(使用Arduino IDE打开)  
4.进入arduino IDE的界面;此时,使用usb数据线将您的模块连接至计算机:  
界面左上角找到长方形框,点击后下拉选中您的usb连接的com口号码,然后在列表中找到Arduino Nano,右边usb需要再次选择一下com口号码 然后按确定  
上方菜单栏:工具/tools > Processror > Atmega168(或328p等 由于不同版本可能用的控制器版本略有差异,若有不成功则可以都试试看)  
以上对arduino IDE的选择设备操作只需做一次,如果更换.ino则需要重新选择  
5.点击左上方第二个叫做 上传 的按钮 等待右下方的提示信息直到提示 上传完成 并重新装回跳线帽 整个上传程序流程完成  


## 版本更新说明
1.三个cv输出的精度提升，减少波纹
2.更新功能说明

