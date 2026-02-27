Midi2CV\_Trs V4.1 使用手册



一、硬件及接口介绍





| 接口类型&#xA;           | 说明&#xA;                                 |
| ------------------- | --------------------------------------- |
| 1x Trs Midi In&#xA; | MIDI 输入接口，可通过跳线盘切换 Trs Type A/B 类型&#xA; |
| 1x Clock Out&#xA;   | 时钟输出接口（可通过 CC24 调整时钟倍速）&#xA;            |
| 2x Note Out&#xA;    | 音符输出接口（CV 范围 0-5V）&#xA;                 |
| 2x Gate Out&#xA;    | 门控输出接口（与音符对应的门控信号，0-5V）&#xA;            |
| 3x CV Out&#xA;      | MIDI CC 信息转换后的 CV 信号输出（0-5V）&#xA;       |



二、硬件使用说明





1.  **电源连接**

    10p 电源接口位于模块上方，连接时需注意红线方向与 10p 灰色排线保持一致（模块内置电源反接保护功能，仍建议规范操作）。


2.  **开机提示**

    模块接通电源后，三盏提示灯将点亮 3 秒，用于确认供电正常。


3.  **跳线设置**

    模块出厂时，跳线帽默认设置为 Trs Type A 类型，可根据您使用的 Trs MIDI 线类型调整。


4.  **信号判断**

*   若 MIDI 设备已开启时钟发送，第一盏提示灯将随 MIDI 时钟信号闪动，可作为 MIDI 信号正常发送的判断依据。


*   当 MIDI 设备发送 Channel 1 的音符时，第二盏灯随 Note On 信号亮起；第三盏灯同理（对应其他通道或功能）。


###### 补充说明&#xA;



*   \*Clock 功能需 MIDI 信号源在设置中开启 “Clock Send（时钟同步发送）”，否则无法接收时钟信号。


*   \*\*Clock Sync 功能需 MIDI 信号源在设置中开启 “Transport Send（走带控制发送）”，否则时钟将自行周期触发，无法随播放同步。


三、软件功能介绍



### 模式切换&#xA;

MIDI 设备可通过 CC10 切换以下 3 种模式（如果你不想修改模式 默认为第一种模式）；若 10 秒内无 MIDI 信号输入，将自动进入概率模式。




| 模式名称 / 接口名称&#xA;     | Clock&#xA;    | Note1&#xA;           | Gate1&#xA;         | Note2&#xA;           | Gate2&#xA;            | CV1&#xA;          | CV2&#xA;              | CV3&#xA;             |
| -------------------- | ------------- | -------------------- | ------------------ | -------------------- | --------------------- | ----------------- | --------------------- | -------------------- |
| Midi: 通道 1&2 模式&#xA; | Clock&#xA;    | Ch1 Note Pitch&#xA;  | Ch1 Note On&#xA;   | Ch2 Note Pitch&#xA;  | Ch2 Note On&#xA;      | Vel1&#xA;         | Vel2&#xA;             | Mod&#xA;             |
| Midi: 通道 3&4 模式&#xA; | Clock1/2&#xA; | Ch3 Note Pitch&#xA;  | Ch3 Note On&#xA;   | Ch4 Note Pitch&#xA;  | Ch4 Note On&#xA;      | Vel1&#xA;         | Vel2&#xA;             | Mod&#xA;             |
| Midi: 复音模式&#xA;      | Clock&#xA;    | Ch1 Poly1 Pitch&#xA; | Ch1 Poly1 On&#xA;  | Ch1 Poly2 Pitch&#xA; | Ch1 Poly2 On&#xA;     | Vel1&#xA;         | Vel2&#xA;             | Mod&#xA;             |
| 概率模式 \*\*\*&#xA;     | Trig In&#xA;  | 75% Rand Trig&#xA;   | 50% Rand Gate&#xA; | 25% Rand Gate&#xA;   | Gate Rand Length&#xA; | Rand Voltage&#xA; | Rand Voltage Inv&#xA; | Rand Voltage1/2&#xA; |

> \*** 注：任意时刻收到 MIDI 信号，将即刻退出概率模式，返回 MIDI 模式。装上D8跳线帽可以使模块开机时自动进入通道3&4模式
>

四、固件更新说明





1.  安装 Arduino IDE（官方下载地址：[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)）。


2.  安装 MIDI 依赖库：下载 [https://github.com/FortySevenEffects/arduino\_midi\_library](https://github.com/FortySevenEffects/arduino_midi_library) 的 ZIP 包，通过 Arduino IDE 的 “库管理器” 导入安装。


3.  下载 Midi2CV 固件程序，使用 Arduino IDE 打开对应的`.ino`文件。


4.  连接设备：用 USB 数据线将模块连接至计算机，在 Arduino IDE 界面左上角的 “设备选择框” 中，下拉选择模块对应的 COM 口；在设备列表中选择 “Arduino Nano”，并确认 COM 口无误。


5.  配置处理器：通过菜单栏 “工具> Processor” 选择对应型号（通常为 Atmega168 或 328p，若上传失败可尝试切换）。


> 注：设备选择操作仅需配置一次，更换
>
> `.ino`
>
> 文件时需重新确认。
>



1.  点击左上角 “上传” 按钮（右箭头图标），等待右下角提示 “上传完成” 后，重新装回跳线帽，完成固件更新。


五、版本更新说明





1.  优化 3 路 CV 输出的精度，减少信号波纹。


2.  完善功能说明文档，补充模式切换及接口映射细节。


> （注：文档部分内容可能由 AI 生成）
>
