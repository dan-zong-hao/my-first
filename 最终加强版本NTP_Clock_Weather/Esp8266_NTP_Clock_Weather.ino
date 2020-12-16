//奇妙之旅开始了，下面的代码可能晦涩难懂，佶屈聱牙，请坐稳扶好
#include <DS18B20.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>         //参看http://www.taichi-maker.com/homepage/iot-development/iot-dev-reference/esp8266-c-plus-plus-reference/
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()


#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"     //OLED 屏的高级图型绘制库，提供图型框架、覆盖层、动画等的高级图型绘制功能，简化OLED的图形操作
#include "Wire.h"
#include "HeFeng.h"

#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"


/***************************
   Begin Settings
 **************************/

DS18B20 ds(D7);


#define TZ              8       // (utc+) TZ in hours时区设置
#define DST_MN          0      // use 60mn for summer time in some countries夏令时

// Setup
const int UPDATE_INTERVAL_SECS = 20 * 60; //每二十分钟更新一次数据
// Setup
const int UPDATE_CURR_INTERVAL_SECS = 10; // 室内温度每10秒更新一次

//显示设置
const int I2C_DISPLAY_ADDRESS = 0x3c;
#if defined(ESP8266)
//引脚定义
const int SDA_PIN = D2;
const int SDC_PIN = D5;
#endif


const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/***************************
   End Settings
 **************************/
// sda-pin=14 and sdc-pin=12
//设置IIC地址和管脚
//https://blog.csdn.net/weixin_33738555/article/details/89658159可以参看这个网站讲解
SSD1306Wire  display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );
//这几个是结构体的声明，看看HeFeng.h
HeFengCurrentData currentWeather;
HeFengForeData foreWeather[3];
HeFeng HeFengClient;

//OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
//OpenWeatherMapForecast forecastClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
//这个可以自己注册一个和风天气
const char* HEFENG_KEY = "60cb7e84e3514c929e912fd049a7df5b";
const char* HEFENG_LOCATION = "beijing";//北京经纬度或就写北京也行，查看和风天气开发者文档就行
//获取UTC时间
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;
long timeSinceLastCurrUpdate = 0;

String currTemp = "-1.0";
//定义 各个图形绘制函数
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();


// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
//定义 FrameCallback frame 图形绘制回调函数数组
FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast };
//drawForecast
int numberOfFrames = 3;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;



bool autoConfig()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin();//这里没有传参数ssid和password就会尝试连接以前连接过的WiFi
  Serial.print("AutoConfig Waiting......");
  int counter = 0;
  for (int i = 0; i < 20; i++)
  {
    //WiFi.status()返回值类型是uint8_t，返回值信息查看http://www.taichi-maker.com/homepage/iot-development/iot-dev-reference/esp8266-c-plus-plus-reference/esp8266wifista/status/
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("AutoConfig Success");
      //WiFi.SSID()返回值是string类型的当前连接网络的SSID
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      //返回密码
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.printDiag(Serial);
      return true;
    }
    else
    {
      delay(500);
      Serial.print(".");
      display.clear();
      display.drawString(64, 10, "Connecting to WiFi");
      display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
      display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
      display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
      display.display();
      counter++;
    }
  }
  Serial.println("AutoConfig Faild!" );
  return false;
}
//以上代码是一插电让esp8266就连接WiFi，定义如果10秒之内还没连上，或者是以前从来没连过网，串口打印一个AutoConfig Faild!，连上了就AutoConfig success


//这个页面是方便配网，省去了重新修改ssid和password再上传的时间，使得代码的到保护
ESP8266WebServer server(80);//默认的服务器端口
//下面是直接将一个html转换成字符串，其中源文件的“”与string的“”构成闭合所以要在每一个“”前面添加上转义字符
String str = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>ESP8266网页配网</title><script type=\"text/javascript\">function wifi(){var ssid = aa.value;var password = bb.value;var xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"/HandleWifi?ssid=\"+ssid+\"&password=\"+password,true);xmlhttp.send();xmlhttp.onload = function(e){alert(this.responseText);}}</script></head><body>hello,这里是程舒璠、何得宇、樊志寒同学的web配网方法,输入wifi信息给wifi时钟配网:  <form>WiFi名称：<input type=\"text\" placeholder=\"请输入您WiFi的名称\" id=\"aa\"><br>WiFi密码：<input type=\"text\" placeholder=\"请输入您WiFi的密码\" id=\"bb\"><br><input type=\"button\" value=\"连接\" onclick=\"wifi()\"></form></body></html>";
void handleRoot() {
  server.send(200, "text/html", str);
}
void HandleWifi()
{
  String wifis = server.arg("ssid"); //从JavaScript发送的数据中找ssid的值
  String wifip = server.arg("password"); //从JavaScript发送的数据中找password的值
  Serial.println("received:" + wifis);
  server.send(200, "text/html", "连接中..");
  WiFi.begin(wifis, wifip);          //链接wifi详细请看http://www.taichi-maker.com/homepage/iot-development/iot-dev-reference/esp8266-c-plus-plus-reference/esp8266wifista/begin/

}


//这里是如果没有访问到这个配网页面的函数
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();//寻找域名
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//下面是web配网的函数，可以参考库里面的样例
void htmlConfig()
{
  WiFi.mode(WIFI_AP_STA);//设置模式为AP+STA
  WiFi.softAP("wifi_clock");//接入点的名称

  IPAddress myIP = WiFi.softAPIP();     //获取ESP8266开发板的IP地址
  //手机不支持mDNS，电脑支持，可以直接访问clock.local
  /*
   * 通过本库，我们可以利用mDNS协议(multicast DNS协议)为ESP8266建立域名访问功能。
   * 也就是说，我们在通过WiFi访问ESP8266时，无需使用ESP8266的IP地址，而可以为ESP8266分配域名并实现访问。
   * 就像我们在访问某一个网址服务器一样。但由于安卓系统不支持mDNS协议，因此使用本库时要慎重。
   */
  if (MDNS.begin("clock")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);//直接访问192.168.4.1的方法
  server.on("/HandleWifi", HTTP_GET, HandleWifi);//访问192.168.4.1/HandleWifi  GET 方式  调取的函数
  server.onNotFound(handleNotFound);//请求失败回调函数
  MDNS.addService("http", "tcp", 80);
  server.begin();//开启服务器
  Serial.println("HTTP server started");
  int counter = 0;
  while (1)
  {
    server.handleClient();
    MDNS.update();//响应mDNS配置
    delay(500);
    display.clear();//下面试一些显示信息
    display.drawString(64, 5, "WIFI AP:wifi_clock");
    display.drawString(64, 20, "192.168.4.1");
    display.drawString(64, 35, "waiting for config wifi.");
    //这就是那个加载的三个点，由上面的delay控制间隔时间，后面那个activeSymbole和inactiveSymbole图案在WeatherStationimages.h里面定义
    display.drawXbm(46, 50, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 50, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 50, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();
    counter++;
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("HtmlConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      Serial.println("HTML连接成功");
      break;
    }
  }
  //关闭服务器
  server.close();
  WiFi.mode(WIFI_STA);

}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  //显示的初始化
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();//屏幕显示翻转
  display.setFont(ArialMT_Plain_10);//字体大小
  display.setTextAlignment(TEXT_ALIGN_CENTER);//设置为居中
  display.setContrast(255);//屏幕亮度
  
//如果autoConfig方法失效就开始web配网
  bool wifiConfig = autoConfig();
  if (wifiConfig == false) {
    htmlConfig();//HTML配网
  }

  ui.setTargetFPS(30);//每秒传输帧数

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  //改为TOP, LEFT, BOTTOM, RIGHT都可 
  ui.setIndicatorPosition(BOTTOM);//显示位置

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);//显示方向

  //改为SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN都可
  ui.setFrameAnimation(SLIDE_LEFT);//滚动方向

  ui.setFrames(frames, numberOfFrames);
  ui.setTimePerFrame(7500);//停留时长
  ui.setOverlays(overlays, numberOfOverlays);

  ui.init();//初始化

  Serial.println("");
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org", "0.cn.pool.ntp.org", "1.cn.pool.ntp.org");//NTP获取时间
  updateData(&display);

}

void loop() {

  if (millis() - timeSinceLastWUpdate > (1000L * UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    //参看81、81行定义
    timeSinceLastWUpdate = millis();
  }
  if (millis() - timeSinceLastCurrUpdate > (1000L * UPDATE_CURR_INTERVAL_SECS)) {
    if ( ui.getUiState()->frameState == FIXED)
    {
      currTemp = String(ds.getTempC(), 1);
      timeSinceLastCurrUpdate = millis();
    }
  }




  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();//屏幕刷新

  if (remainingTimeBudget > 0) {
    // 预留了一部分空间可以写点别的函数
    delay(remainingTimeBudget);
  }
///////////////////////////////////////////////////////////////////////
}
//画进度条
void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);//进度条
  display->display();
}


void updateData(OLEDDisplay *display) {
  drawProgress(display, 30, "Updating weather...");

  for (int i = 0; i < 5; i++) {
    HeFengClient.doUpdateCurr(&currentWeather, HEFENG_KEY, HEFENG_LOCATION);
    if (currentWeather.cond_txt != "no network") {
      break;
    }
  }
  drawProgress(display, 50, "Updating forecasts...");

  for (int i = 0; i < 5; i++) {
    HeFengClient.doUpdateFore(foreWeather, HEFENG_KEY, HEFENG_LOCATION);
    if (foreWeather[0].datestr != "N/A") {
      break;
    }
  }

  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(1000);
}


//时间函数
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];


  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  String date = WDAY_NAMES[timeInfo->tm_wday];

  sprintf_P(buff, PSTR("%04d-%02d-%02d, %s"), timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, WDAY_NAMES[timeInfo->tm_wday].c_str());
  display->drawString(64 + x, 5 + y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 22 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}
//实时天气界面函数
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, currentWeather.cond_txt + " | Wind: " + currentWeather.wind_sc);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = currentWeather.tmp + "°C" ;
  display->drawString(60 + x, 3 + y, temp);
  display->setFont(ArialMT_Plain_10);
  display->drawString(70 + x, 26 + y, currentWeather.fl + "°C | " + currentWeather.hum + "%");
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);
}

//天气预报界面
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, foreWeather[dayIndex].datestr);
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, foreWeather[dayIndex].iconMeteoCon);

  String temp = foreWeather[dayIndex].tmp_min + " | " + foreWeather[dayIndex].tmp_max;
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = currTemp + "°C";
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}
