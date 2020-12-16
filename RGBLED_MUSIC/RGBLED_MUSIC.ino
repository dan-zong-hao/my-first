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
    if (i < 20) {
      //第一节的三套
      if (time % 2000 == 0) {
        pixels.setPixelColor(i, pixels.Color(148, 0, 211));
      } else if (time % 3000 == 0) {
        pixels.setPixelColor(i, pixels.Color(139, 28, 98));
      } else if (time % 5000 == 0) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      } 
    } else if (i >= 21 && i < 38) {
      //第二节的三套
      if (time % 2000 == 0) {
        pixels.setPixelColor(i, pixels.Color(106, 90, 205));
      } else if(time % 3000 == 0){
        pixels.setPixelColor(i, pixels.Color(205, 41, 144));
      } else if(time % 5000 == 0){
        pixels.setPixelColor(i, pixels.Color(152， 251， 152));
      }
    } else if (i >= 39 && i < 56) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 205));
    } else if (i >= 57 && i < 74) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    } else if (i >= 75 && i < 92) {
      pixels.setPixelColor(i, pixels.Color(30, 144, 255));
    } else if (i >= 93 && i < 110) {
      pixels.setPixelColor(i, pixels.Color(0, 191, 255));
    } else if (i >= 111 && i < 128) {
      pixels.setPixelColor(i, pixels.Color(0, 255, 255));
    } else {
      pixels.setPixelColor(i, pixels.Color(127, 255, 212));
    }
  }



  //点亮对应数量的灯珠
  pixels.show();
  //将所有灯珠状态发送到灯条
}
