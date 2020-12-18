#include <Adafruit_NeoPixel.h>
#define PIN       6
//RGB灯带信号引脚
#define MICPIN    A0
//麦克风传感器信号引脚
#define NUMPIXELS 144
//RGB灯带灯珠总数
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 50
//灯珠间时延，单位为毫秒
float sound = 0;
//定义声音响度并初始化为0
int soundLED = 0;
//定义点亮灯珠数量并初始化为0
void setup() {
  pixels.begin();
  //初始化灯带
}
//定义时间
int time = millis();
void loop() {
  sound = (float)analogRead(MICPIN) / 800 * NUMPIXELS;
  //将传感器的响度值转换为亮起的灯珠数量（浮点值）
  soundLED = (int)sound;
  //转化为整数
  pixels.clear();
  //清除所有灯珠的状态


  for (int i = 0; i < sound; i++) {
    pixels.setPixelColor(i, pixels.Color(148, 0, 211));
  }
        
    
//这里面可以用if判断来设置每一段等待的颜色都可以自己定义，效果如视频所示，上网查找你喜欢的颜色的rgb值


  //点亮对应数量的灯珠
  pixels.show();
  //将所有灯珠状态发送到灯条
}
