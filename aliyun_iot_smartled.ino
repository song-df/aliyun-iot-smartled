#include <Arduino.h>
#include "pins_config.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "lvgl.h"

// 引入阿里云 IoT SDK
#include <AliyunIoTSDK.h>

static WiFiClient espClient;

// 设置产品和设备的信息，从阿里云设备信息里查看
#define PRODUCT_KEY "a1w4Q3qdnUk"
#define DEVICE_NAME "ESP32_virtual_LED1"
#define DEVICE_SECRET "56d811b0d1e1a98ea15cf7e66466e61a"
#define REGION_ID "cn-shanghai"


#ifndef STASSID
#define STASSID "Xiaomi_Jack_2.4G"
#define STAPSK  "135792460"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;
static bool led_status = false;

WebServer server(80);
void handleRoot() {
  digitalWrite(PIN_LED, HIGH);
  server.send(200, "text/plain", "hello from esp32!");
  digitalWrite(PIN_LED, LOW);
}

void handleLedOn(){
  digitalWrite(PIN_LED, HIGH);
  led_status = true;
  server.send(200, "text/plain", "esp32 led is on!");
  AliyunIoTSDK::send("LightSwitch", led_status?1:0);
}

void handleLedOff(){
  digitalWrite(PIN_LED, LOW);
  led_status = false;
  server.send(200, "text/plain", "esp32 led is off!");
  AliyunIoTSDK::send("LightSwitch", led_status?1:0);
}

void handleNotFound() {
  digitalWrite(PIN_LED, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(PIN_LED, LOW);
}

void led_task(void *param);
// 电源属性修改的回调函数
void powerCallback(JsonVariant p)
{
    int PowerSwitch = p["LightSwitch"];
    Serial.printf("led switch: %d\n",PowerSwitch);
    if (PowerSwitch == 1)
    {
        // 启动设备
        led_status = true;
        digitalWrite(PIN_LED,HIGH);
    } 
     if (PowerSwitch == 0)
    {
        // 关闭设备
        led_status = false;
        digitalWrite(PIN_LED,LOW);
    } 
    AliyunIoTSDK::send("LightSwitch", led_status?1:0);
}

void setup() {
  // put your setup code here, to run once:
  
  pinMode(PIN_LED,OUTPUT);
  Serial.begin(115200);
  while(!Serial){}

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

// Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  //xTaskCreate(led_task, "LED Flash Task", 2048, NULL, tskIDLE_PRIORITY, NULL);

// 初始化 iot，需传入 wifi 的 client，和设备产品信息
    AliyunIoTSDK::begin(espClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
    
    // 绑定一个设备属性回调，当远程修改此属性，会触发 powerCallback
    // PowerSwitch 是在设备产品中定义的物联网模型的 id
    AliyunIoTSDK::bindData("LightSwitch", powerCallback);
    
    // 发送一个数据到云平台，LightLuminance 是在设备产品中定义的物联网模型的 id
    AliyunIoTSDK::send("LightSwitch", led_status?1:0);

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/ledon", handleLedOn);
  server.on("/ledoff", handleLedOff);
  

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  // put your main code here, to run repeatedly:

  server.handleClient();
  delay(2);

    AliyunIoTSDK::loop();
}



void led_task(void *param)
{
  pinMode(PIN_LED,OUTPUT);
    while (true)
    {
        digitalWrite(PIN_LED, 1);
        delay(20);

        digitalWrite(PIN_LED, 0);
        delay(980);
    }
}
