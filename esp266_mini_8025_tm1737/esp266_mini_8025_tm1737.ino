/***
 * ESP8266 RX8025T TM1637 NTP 表
 *
 * 使用的库  ; Grove 4  Digit Display; RX8025_RTC ；WiFiManager
 * **/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <TimeLib.h>
#include <Wire.h>
#include "RX8025_RTC.h"

#include <time.h>     // time() ctime()
#include <sys/time.h> // struct timeval

#define SECOND_ADJUSTMENT -1

extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

// NTP Config
int timezone = 8 * 3600; //东8区
int dst = 0;             //夏令时
int y = 1970, mon, d, h, m, s, wk;

uint32_t chipId = ESP.getChipId();
//uint32_t now_ms, now_us;

RX8025_RTC rtc_rx8025;
tmElements_t tm;

timeval tv;
timespec tp;

uint32_t now_ms, now_us;

/***
 *
 tm1637
 *
 * **/
#include <TM1637.h>
#define ON 1
#define OFF 0
//#define SECOND_ADJUSTMENT 16
int8_t TimeDisp[] = {0x00, 0x00, 0x00, 0x00};

volatile unsigned char ClockPoint = 1;
volatile long en_esp = 0; //esp01 使能
//unsigned char Update= ON;
//unsigned char halfsecond = 0;
//unsigned char second;
unsigned char lcd_minutes = 0;
unsigned char lcd_hours = 12;

unsigned int esp_time = 60 * 1;               //1分钟 esp8266通电时间
unsigned long re_esp_time = 12 * 60 * 60 * 1; //esp8266通电时间 12小时

//#define CLK 6 //pins definitions for TM1637 and can be changed to other ports
//#define DIO 7

#define CLK 14 //pins definitions for TM1637 and can be changed to other ports
#define DIO 12
TM1637 tm1637(CLK, DIO);

/***
 *

 *
 * **/

//更新esp8266 rtc 时间
void ntp_set_esp8266_time()
{
  configTime(timezone, dst, "time.pool.aliyun.com", "time1.cloud.tencent.com",
             "cn.ntp.org.cn");
  //  Serial.print("all");
  //while (!time(nullptr)) {
  delay(150);
  //}
  if (!getNTPtime())
  {
    configTime(timezone, dst, "2.debian.pool.ntp.org",
               "1.debian.pool.ntp.org", "0.debian.pool.ntp.org");
    //  Serial.print("debian");
    delay(150);
  };

  if (!getNTPtime())
  {

    configTime(timezone, dst, "2.ubuntu.pool.ntp.org",
               "1.ubuntu.pool.ntp.org", "0.ubuntu.pool.ntp.org");
    //  Serial.print("ubuntu");
    delay(150);
  };
  if (!getNTPtime())
  {

    configTime(timezone, dst, "1.centos.pool.ntp.org",
               "2.centos.pool.ntp.org", "0.centos.pool.ntp.org");
    //  Serial.print("ubuntu");
    delay(150);
  };
};

//查询是否同步成功
bool getNTPtime()
{
  {
    print_time();
    if (y <= 2019)
      return false; // the NTP call was not successful

    ////  Serial.print("Time Now: ");
    //  //  Serial.println(now);
  }
  return true;
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000L);

  // Connect WiFi
  WiFiManager wifiManager;
  //Serial.begin(9600);
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
  if (!wifiManager.autoConnect("ESP_AP"))
  {
    //  Serial.println(F("Failed to connect. Reset and try again. . ."));
    delay(3000);
    //  ESP.wdtFeed();
    //重置并重试
    ESP.reset();
    // ESP.wdtFeed();
    delay(5000);
    //  ESP.wdtFeed();
  }

  ESP.wdtEnable(WDTO_4S); //打开看门狗
  //在我们从网络浏览器中选择Wi-Fi网络和密码后，我们现在连接到Wi-Fi网络：

  //如果你到这里，你已经连接到WiFi
  //  Serial.println(F("Connected to Wifi."));
  //  Serial.print(F("My IP:"));
  //  Serial.println(WiFi.localIP());

  // Get time from NTP server
  rest_esp8266_date();
  ntp_set_esp8266_time();

  if (!getNTPtime())
  { //如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };
  if (!getNTPtime())
  { //如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };
  if (!getNTPtime())
  { //如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };
  if (!getNTPtime())
  { //如果上一轮失败再做一次同步
    ntp_set_esp8266_time();
  };

  //print_time(); //初始化一下变量 好改时间
  //
  //delay(1000);
  if (getNTPtime())
  {
    clockrx8025set(); //设置rtc_rx8025 RTC 时间
  };
  ESP.wdtFeed();
  /**
   tm1637 **/

  tm1637.set(5); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

  tm1637.init();
  //initDateTime();
  // rtc.write(tm);
  //Timer1.initialize(2*500000);         //timing for 500ms
  // Timer1.attachInterrupt(TimingISR); //declare the interrupt serve routine:TimingISR
  tm1637.display(TimeDisp);
  ESP.wdtFeed();
  /**
   tm1637 end **/

  //  ESP.deepSleep(2 * 60 * 60e6);  //休眠
}

void loop()
{

  //  time_t now = time(nullptr);
  ////  Serial.println(now);
  //clock1307set();
  //// print_time();

  //print_time(); //打印ESP8266 RTC 时间

  //tmElements_t tm2;
  //tm2 = rtc_rx8025.read();
  //char s[20];
  //sprintf(s, "%d/%d/%d %d:%d:%d", tmYearToCalendar(tm2.Year), tm2.Month,
  //tm2.Day, tm2.Hour, tm2.Minute, tm2.Second);
  //Serial.println(s);

  if (en_esp == re_esp_time)
  {

    ntp_set_esp8266_time();

    if (!getNTPtime())
    { //如果上一轮失败再做一次同步
      ntp_set_esp8266_time();
    };
    if (!getNTPtime())
    { //如果上一轮失败再做一次同步
      ntp_set_esp8266_time();
    };
    if (!getNTPtime())
    { //如果上一轮失败再做一次同步
      ntp_set_esp8266_time();
    };
    if (!getNTPtime())
    { //如果上一轮失败再做一次同步
      ntp_set_esp8266_time();
    };

    //print_time(); //初始化一下变量 好改时间
    //
    //delay(1000);
    if (getNTPtime())
    {
      clockrx8025set(); //设置rtc_rx8025 RTC 时间
    };
  }
  ESP.wdtFeed();
  en_esp++;
  if (en_esp == re_esp_time)
  {
    en_esp = 0;
  }
  TimingISR();
  TimeUpdate();
  tm1637.display(TimeDisp);
  delay(1000);
  ESP.wdtFeed();

  // ESP.deepSleep(2 * 60 * 60e6); //休眠
}

//打印ESP8266 RTC 时间
void print_time()
{
  time_t now = time(nullptr);
  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);

  //  Serial.print(now.yr, now.mon, now.date, now.hr, now.min, now.sec);
  //Serial.print(now);
  y = 1970;
  wk = ((now / 86400L) % 7 + 4) % 7; //86400 is secons in one day; +1 for 1970/1/1 is 周四
  do
  {
    unsigned long ys;
    if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
    {
      ys = 31622400L; //31622400 = 366 * 24 * 3600;
    }
    else
    {
      ys = 31536000L; // 31536000 = 365 * 24 * 3600;
    }
    if (now < (signed long)ys)
    {
      break;
    }
    else
    {
      now -= ys;
      y++;
    }
  } while (1);

  int mons[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
  {
    mons[1] = 29;
  }
  for (mon = 0; mon < 12; mon++)
  {
    if (now < mons[mon] * 86400L)
    {
      break;
    }
    else
    {
      now -= mons[mon] * 86400L;
    }
  }

  d = now / 86400L + 1; //86400 = 24 * 3600 = how many seconds in a day
  now = now % 86400L;

  h = now / 3600;
  now = now % 3600;
  m = now / 60;
  s = now % 60;

  //char buf[50];
  //snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d %0d", y, mon + 1,
  //    d, h, m, s, wk);
  //Serial.print("DATE: ");
  //Serial.println(buf);
}

void clockrx8025set()
{
  print_time();
  tm.Year = CalendarYrToTm(y);
  tm.Month = mon + 1;
  tm.Day = d;
  tm.Hour = h;
  tm.Minute = m;
  tm.Second = s + SECOND_ADJUSTMENT;
  //char s[20];
  //sprintf(s, "write %d/%d/%d %d:%d:%d", tmYearToCalendar(tm.Year), tm.Month,
  //  tm.Day, tm.Hour, tm.Minute, tm.Second);
  //Serial.println(s);
  rtc_rx8025.write(tm); //write time to the RTC chip
  ///把EPS8266的时间重置掉
  rest_esp8266_date();
};

void rest_esp8266_date()
{
  unsigned long epoch = 0001;
  time_t rtc = epoch;
  timeval tv = {rtc, 0};
  settimeofday(&tv, 0);
}

/****
 * tm1637
 *
 * ***/

void TimingISR()
{

  ClockPoint = !ClockPoint;
}

bool initDateTime()
{
  const char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char mon[12];
  int Year, Month, Day, Hour, Minute, Second;

  if (sscanf(__DATE__, "%s %d %d", mon, &Day, &Year) != 3)
  {
    return false;
  }
  if (sscanf(__TIME__, "%d:%d:%d", &Hour, &Minute, &Second) != 3)
  {
    return false;
  }

  uint8_t idx;
  Month = 0;
  for (idx = 0; idx < 12; idx++)
  {
    if (strcmp(mon, monthNames[idx]) == 0)
    {
      Month = idx + 1;
      break;
    }
  }
  if (Month == 0)
  {
    return false;
  }
  Second += SECOND_ADJUSTMENT;
  tm.Year = CalendarYrToTm(Year);
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
  return true;
}

//8025读取打印时间
void printTime()
{
  tmElements_t tm2;
  tm2 = rtc_rx8025.read();

  lcd_hours = (tm2.Hour);
  // Serial.println(tm2.Hour); // 串口输出唯一ID
  lcd_minutes = (tm2.Minute);
  //Serial.println(tm2.Minute); // 串口输出唯一ID
}
void TimeUpdate(void)
{
  if (ClockPoint)
    tm1637.point(POINT_ON);
  else
    tm1637.point(POINT_OFF);
  printTime(); //设置数据 准备显示时间
  // Update = OFF;

  TimeDisp[0] = lcd_hours / 10;
  TimeDisp[1] = lcd_hours % 10;
  TimeDisp[2] = lcd_minutes / 10;
  TimeDisp[3] = lcd_minutes % 10;
}

/****
 * tm1637
 *
 * ***/
