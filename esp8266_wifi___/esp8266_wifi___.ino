#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
//使用库  WiFiManager
//实例化 wifiManager 对象。这将在访问点模式下启动ESP，并将一个强制门户网站重定向到配置网页：
void setup() {
  WiFiManager wifiManager;
  Serial.begin(115200);
  //reset saved settings
  //wifiManager.resetSettings();

  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  //  wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

//将到期超时设置为240秒：

  wifiManager.setConfigPortalTimeout(240);
  if (!wifiManager.autoConnect("ESP_AP", "changeit")) {
    Serial.println(F("Failed to connect. Reset and try again. . ."));
    delay(3000);
    //重置并重试
    ESP.reset();
    delay(5000);
  }

//在我们从网络浏览器中选择Wi-Fi网络和密码后，我们现在连接到Wi-Fi网络：

  //如果你到这里，你已经连接到WiFi
  Serial.println(F("Connected to Wifi."));
  Serial.print(F("My IP:"));
  Serial.println(WiFi.localIP());
}

void loop() {
  //为loop（）添加你的代码
}
