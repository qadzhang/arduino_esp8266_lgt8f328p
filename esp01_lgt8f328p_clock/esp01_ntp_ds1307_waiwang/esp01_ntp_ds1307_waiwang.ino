#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <time.h>
#include "DS1307.h"
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

// NTP Config
int timezone = 8 * 3600; //东8区
int dst = 0; //夏令时
int y = 1970, mon, d, h, m, s, wk;
timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;
DS1307 clock1307; //define a object of DS1307 class
void setup() {

  // Connect WiFi
  WiFiManager wifiManager;
 // Serial.begin(115200);
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
  // if (!wifiManager.autoConnect("ESP_AP", "changeit")) {
  if (!wifiManager.autoConnect("ESP_AP")) {
    //  Serial.println(F("Failed to connect. Reset and try again. . ."));
    delay(3000);
    //重置并重试
    ESP.reset();
    delay(5000);
  }

  //在我们从网络浏览器中选择Wi-Fi网络和密码后，我们现在连接到Wi-Fi网络：

  //如果你到这里，你已经连接到WiFi
  //  Serial.println(F("Connected to Wifi."));
  //  Serial.print(F("My IP:"));
  //  Serial.println(WiFi.localIP());

  clock1307.begin();
  clock1307.startClock();

  // Get time from NTP server

  ntp_set_esp8266_time();

  if (!getNTPtime()) {//如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };  if (!getNTPtime()) {//如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };  if (!getNTPtime()) {//如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };  if (!getNTPtime()) {//如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };
//print_time(); //初始化一下变量 好改时间
  //
  //delay(1000);
  if (getNTPtime()) {
    clock1307set();
  };

 ESP.deepSleep(2*60*60e6 );//休眠
}

//更新esp8266 rtc 时间
void ntp_set_esp8266_time() {
  configTime(timezone, dst, "time.pool.aliyun.com", "time1.cloud.tencent.com",
      "cn.ntp.org.cn");
  //  Serial.print("all");
  //while (!time(nullptr)) {
  delay(200);
  //}
  if (!getNTPtime()) {
    configTime(timezone, dst, "2.debian.pool.ntp.org",
        "1.debian.pool.ntp.org", "0.debian.pool.ntp.org");
    //  Serial.print("debian");
    delay(200);
  };

  if (!getNTPtime()) {

    configTime(timezone, dst, "2.ubuntu.pool.ntp.org",
        "1.ubuntu.pool.ntp.org", "0.ubuntu.pool.ntp.org");
    //  Serial.print("ubuntu");
    delay(200);
  };

}
void loop() {

//  time_t now = time(nullptr);
  ////  Serial.println(now);
  //clock1307set();
 //// print_time();

 // printTime();
 //delay(1000);
   ESP.deepSleep(2*60*60e6 );//休眠
}

//查询是否同步成功
bool getNTPtime() {
  {

    print_time();

    if (y <= 2019)
      return false;  // the NTP call was not successful

    ////  Serial.print("Time Now: ");
  //  //  Serial.println(now);
  }
  return true;
}

//打印ESP8266 RTC 时间
void print_time() {
  now = time(nullptr);
  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);

  // //  Serial.print(now.yr, now.mon, now.date, now.hr, now.min, now.sec);

  y = 1970;
  wk = ((now / 86400L) % 7 + 4) % 7; //86400 is secons in one day; +1 for 1970/1/1 is 周四
  do {
    unsigned long ys;
    if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
      ys = 31622400L; //31622400 = 366 * 24 * 3600;
    } else {
      ys = 31536000L; // 31536000 = 365 * 24 * 3600;
    }
    if (now < (signed long) ys) {
      break;
    } else {
      now -= ys;
      y++;
    }
  } while (1);

  int mons[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
    mons[1] = 29;
  }
  for (mon = 0; mon < 12; mon++) {
    if (now < mons[mon] * 86400L) {
      break;
    } else {
      now -= mons[mon] * 86400L;
    }
  }

  d = now / 86400L + 1; //86400 = 24 * 3600 = how many seconds in a day
  now = now % 86400L;

  h = now / 3600;
  now = now % 3600;
  m = now / 60;
  s = now % 60;

  char buf[50];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d %0d", y, mon + 1,
      d, h, m, s, wk);
  //  Serial.print("DATE: ");
  //  Serial.println(buf);
}

void clock1307set() {
  print_time();
  clock1307.fillByYMD(y, mon, d); //Jan 19,2013
  clock1307.fillByHMS(h, m, s + 1);   //15:28 30"

  switch (wk) // Friendly printout the weekday
  {
  case 1:
    clock1307.fillDayOfWeek(MON);
    break;
  case 2:
    clock1307.fillDayOfWeek(TUE);
    break;
  case 3:
    clock1307.fillDayOfWeek(WED);
    break;
  case 4:
    clock1307.fillDayOfWeek(THU);
    break;
  case 5:
    clock1307.fillDayOfWeek(FRI);
    break;
  case 6:
    clock1307.fillDayOfWeek(SAT);
    break;
  default:
    clock1307.fillDayOfWeek(SUN);
    break;
  }

  clock1307.setTime(); //write time to the RTC chip
}
//打印DS1307 RTC 时间
void printTime() {
  clock1307.getTime();

  //  Serial.print(clock1307.hour, DEC);
  //  Serial.print(":");
  //  Serial.print(clock1307.minute, DEC);
  //  Serial.print(":");
  //  Serial.print(clock1307.second, DEC);
  //  Serial.print("  ");
  //  Serial.print(clock1307.month, DEC);
  //  Serial.print("/");
  //  Serial.print(clock1307.dayOfMonth, DEC);
  //  Serial.print("/");
  //  Serial.print(clock1307.year + 2000, DEC);
  //  Serial.print(" ");
  //  Serial.print(clock1307.dayOfMonth);
  //  Serial.print("*");
  switch (clock1307.dayOfWeek) // Friendly printout the weekday
  {
  case MON:
    //  Serial.print("MON");
    break;
  case TUE:
    //  Serial.print("TUE");
    break;
  case WED:
    //  Serial.print("WED");
    break;
  case THU:
    //  Serial.print("THU");
    break;
  case FRI:
    //  Serial.print("FRI");
    break;
  case SAT:
    //  Serial.print("SAT");
    break;
  case SUN:
    //  Serial.print("SUN");
    break;
  }
  //  Serial.println(" ");
}
