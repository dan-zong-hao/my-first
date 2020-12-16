#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>  
#include <WiFiClientSecureBearSSL.h>  //BearSSL客户端安全和服务器安全
#include "HeFeng.h"
//Warning！！！！
//这里面是解析json的代码
HeFeng::HeFeng() {

}




void HeFeng::doUpdateCurr(HeFengCurrentData *data, String key,String location) {
    /*************************************************************************************
     下面这玩意叫智能指针std::unique_ptr，绑定了一个动态的对象---“要想创建一个unique_ptr，
     我们需要将一个new 操作符返回的指针传递给unique_ptr的构造函数。”
     *************************************************************************************/
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);    
    client->setInsecure();    //setInsecure()ESP8266将不会进行任何服务器身份认证而直接与服务器进行通讯，所以此方法不适合传输需要保密的信息。
    
    HTTPClient https;                 //创建https对象  
    String url="https://free-api.heweather.net/s6/weather/now?lang=en&location="+location+"&key="+key;
  Serial.print("[HTTPS] begin...\n");  
    if (https.begin(*client, url)) {               // HTTPS我觉得这句话里面这个*client应该取了个寂寞，示例代码的setFingerprint函数是有返回值的   
      /*
       * 这个begin函数返回值是bool
       * 参看示例代码
       */
      // 开始连接并发送请求头
      int httpCode = https.GET(); //GET方法从指定的资源请求数据，返回一个服务器状态码（返回值类型：int类型）错误时httpCode会变成负数
      if (httpCode > 0) {  
        // http header已发送，服务器响应header已处理
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);    
        /*
         * 下面这个"HTTP_CODE_OK"已经在<ESP8266HTTPClient.h>里面定义好了状态码-200代表请求成功
         * 下面这个"HTTP_CODE_MOVED_PERMANENTLY"对应-301: 资源（网页等）被永久转移到其它URL
         */
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  
          String payload = https.getString();  
          Serial.println(payload);
          /*
           * JsonDocuments是整个ArduinoJson库的内存入口，负责处理整个json数据的内存管理
           * DynamicJsonDocment是分配在heap区，自动回收
           */
          DynamicJsonDocument  jsonBuffer(2048); //获取顶节点，并把它转成T类型
          deserializeJson(jsonBuffer, payload);   //解析json
          /*
           * 这段我不懂只能用例子演示
           * DynamicJsonBuffer doc(1024);
           * deserializeJson(doc, "{\"key\":\"value\")");
           * // get the JsonObject in the JsonDocument
           * JsonObject root = doc.as<JsonObject>()；
           * // get the value in the JsonObject
           * const char* value = root["key"];
           */
          JsonObject root = jsonBuffer.as<JsonObject>();//https://blog.csdn.net/dpjcn1990/article/details/92831648
          
         String tmp=root["HeWeather6"][0]["now"]["tmp"];
         //这里的data是函数的参数是个指针变量，用来访问结构体里面的变量，参看HeFeng.h        
         data->tmp=tmp;             
         String fl=root["HeWeather6"][0]["now"]["fl"];         
         data->fl=fl;
         String hum=root["HeWeather6"][0]["now"]["hum"];         
         data->hum=hum;
         String wind_sc=root["HeWeather6"][0]["now"]["wind_sc"];         
         data->wind_sc=wind_sc;
         String cond_code=root["HeWeather6"][0]["now"]["cond_code"];  
         String meteoconIcon=getMeteoconIcon(cond_code);
         String cond_txt=root["HeWeather6"][0]["now"]["cond_txt"];
         data->cond_txt=cond_txt;
         data->iconMeteoCon=meteoconIcon;
         
        }  
      } else {  
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
         data->tmp="-1";                 
         data->fl="-1";       
         data->hum="-1";      
         data->wind_sc="-1";         
         data->cond_txt="no network";
         data->iconMeteoCon=")";
      }  
      //调用此函数来清除ESP8266的接收缓存以便设备再次接收服务器发来的响应信息。
      https.end();  
    } else {  
      Serial.printf("[HTTPS] Unable to connect\n");
         data->tmp="-1";                 
         data->fl="-1";       
         data->hum="-1";      
         data->wind_sc="-1";         
         data->cond_txt="no network";
         data->iconMeteoCon=")";
    }  

}

void HeFeng::doUpdateFore(HeFengForeData *data, String key,String location) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);    
    client->setInsecure();    
    HTTPClient https;  
    String url="https://free-api.heweather.net/s6/weather/forecast?lang=en&location="+location+"&key="+key;
  Serial.print("[HTTPS] begin...\n");  
    if (https.begin(*client, url)) {  // HTTPS    
      // start connection and send HTTP header  
      int httpCode = https.GET();    
      // httpCode will be negative on error  
      if (httpCode > 0) {  
        // HTTP header has been send and Server response header has been handled  
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);    
     
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  
          String payload = https.getString();  
          Serial.println(payload);  
          DynamicJsonDocument  jsonBuffer(8192);
          deserializeJson(jsonBuffer, payload);
          JsonObject root = jsonBuffer.as<JsonObject>();
        int i;
           for (i=0; i<3; i++){
         String tmp_min=root["HeWeather6"][0]["daily_forecast"][i]["tmp_min"];         
         data[i].tmp_min=tmp_min;             
         String tmp_max=root["HeWeather6"][0]["daily_forecast"][i]["tmp_max"];         
         data[i].tmp_max=tmp_max;  
          String datestr=root["HeWeather6"][0]["daily_forecast"][i]["date"];         
         data[i].datestr=datestr.substring(5,datestr.length()); 
         String cond_code=root["HeWeather6"][0]["daily_forecast"][i]["cond_code_d"];  
         String meteoconIcon=getMeteoconIcon(cond_code);        
         data[i].iconMeteoCon=meteoconIcon;
           }
        }  
      } else {  
        //errorToString函数可用于将ESP8266的HTTP请求失败代码转换为字符串描述。
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());  
          int i;
           for (i=0; i<3; i++){               
         data[i].tmp_min="-1";          
         data[i].tmp_max="-1";    
         data[i].datestr="N/A";
         data[i].iconMeteoCon=")";
           }
      }  
  
      https.end();  
    } else {  
      Serial.printf("[HTTPS] Unable to connect\n");  
        int i;
           for (i=0; i<3; i++){               
         data[i].tmp_min="-1";          
         data[i].tmp_max="-1";    
         data[i].datestr="N/A";
         data[i].iconMeteoCon=")";
           }
    }  

}
   //
   String HeFeng::getMeteoconIcon(String cond_code)
   {
    if(cond_code=="100"||cond_code=="9006"){return "B";}//晴
    if(cond_code=="999"){return ")";}                   //未知
    if(cond_code=="104"){return "D";}                   //阴
    if(cond_code=="500"){return "E";}                   //薄雾，一下的cond_code对应天气查看和风天气开发者文档https://dev.qweather.com/docs/start/icons/
    if(cond_code=="503"||cond_code=="504"||cond_code=="507"||cond_code=="508"){return "F";}
    if(cond_code=="499"||cond_code=="901"){return "G";}
    if(cond_code=="103"){return "H";}
    if(cond_code=="502"||cond_code=="511"||cond_code=="512"||cond_code=="513"){return "L";}
    if(cond_code=="501"||cond_code=="509"||cond_code=="510"||cond_code=="514"||cond_code=="515"){return "M";}
    if(cond_code=="102"){return "N";}
    if(cond_code=="213"){return "O";}
    if(cond_code=="302"||cond_code=="303"){return "P";}
    if(cond_code=="305"||cond_code=="308"||cond_code=="309"||cond_code=="314"||cond_code=="399"){return "Q";}
    if(cond_code=="306"||cond_code=="307"||cond_code=="310"||cond_code=="311"||cond_code=="312"||cond_code=="315"||cond_code=="316"||cond_code=="317"||cond_code=="318"){return "R";}
    if(cond_code=="200"||cond_code=="201"||cond_code=="202"||cond_code=="203"||cond_code=="204"||cond_code=="205"||cond_code=="206"||cond_code=="207"||cond_code=="208"||cond_code=="209"||cond_code=="210"||cond_code=="211"||cond_code=="212"){return "S";}
    if(cond_code=="300"||cond_code=="301"){return "T";}
    if(cond_code=="400"||cond_code=="408"){return "U";}
    if(cond_code=="407"){return "V";}
    if(cond_code=="401"||cond_code=="402"||cond_code=="403"||cond_code=="409"||cond_code=="410"){return "W";}
    if(cond_code=="304"||cond_code=="313"||cond_code=="404"||cond_code=="405"||cond_code=="406"){return "X";}
    if(cond_code=="101"){return "Y";}
    return ")";   
    }    
 
