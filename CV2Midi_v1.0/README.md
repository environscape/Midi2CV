# CV2Midi V1.0 使用手册

## 硬件及接口介绍:  
1xTrs Midi Out
8xCV In

## 软件功能介绍:   
您可以配置midi消息类型 音符编号 音符开关 CC消息编号 
并且每一个都可以配置1-16的midi通道
最终他们都以midi信息的方式从Trs Midi接口发送出去

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



