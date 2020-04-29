/*********************************************************************** 
//   ESP8266 网络同步时间后设置DS1307时间
//用到的库
//  Grove - RTC DS1307;
//这个应该是可以不断的NTP的，原来那个到底是编译器BUG还是写的问题，不知道了。。。。

因为用的ESP01 使用RX TX做I2C，而开发板使用的d1 mini的模板，所以 从这里改下定义

 .arduino15/packages/esp8266/hardware/esp8266/2.6.3/variants/d1_mini/
改成如下的： RX作为SCL TX作为SDA 
#define PIN_WIRE_SDA (1)
#define PIN_WIRE_SCL (3)

d1 mini 默认是
#define PIN_WIRE_SDA (4)
#define PIN_WIRE_SCL (5)
 *************************************************************************/
#include <Wire.h>
#include "DS1307.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>     // time() ctime()
#include <sys/time.h> // struct timeval

#ifndef STASSID
#define STASSID "zc_hm"
#define STAPSK "zc_hofdgfd69"
#define TZ 7      // (utc+) TZ in hours  东8区为啥是+7 ？？？
#define DST_MN 60 // use 60mn for summer time in some countries

#define TZ_MN ((TZ)*60)
#define NTP0_OR_LOCAL1 1    // 0:use NTP  1:fake external RTC
#define RTC_TEST 1510592825 // 1510592825 = Monday 13 November 2017 17:07:05 UTC

////////////////////////////////////////////////////////

#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)
#endif
//#define PIN_WIRE_SDA (1)
//#define PIN_WIRE_SCL (3)

//#define uint8_t SDA = PIN_WIRE_SDA;
//#define uint8_t SCL = PIN_WIRE_SCL;
// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

//unsigned long  time_us = 2 * 60 * 1000000; //睡眠时间

const char *ssid = STASSID; // your network SSID (name)
const char *pass = STAPSK;  // your network password
int conts = 0;              //计数器
boolean if_ntp = false;     //是否同步NTP
uint32_t chipId = ESP.getChipId();
unsigned int localPort = 2390; // local port to listen for UDP packets
int y = 1970, mon, d, h, m, s, wk;
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 Lookup the IP address for the host name instead */
IPAddress timeServerIP(192, 168, 198, 126); // time.nist.gov NTP server
//IPAddress timeServerIP; // time.nist.gov NTP server address
const char *ntpServerName = "ntp1.aliyun.com";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp; // @suppress("Abstract class cannot be instantiated")

DS1307 clock1307; //define a object of DS1307 class
void setup() {
//Wire.setClock(400000);//设置I2C速度为400MHZ
 // pinMode(LED_BUILTIN, OUTPUT);
 // ESP.wdtEnable(WDTO_8S); //打开看门狗

  //Serial.begin(115200);
  //  // Serial.println();
  //  // Serial.println();

  // We start by connecting to a WiFi network
  //  // Serial.print("Connecting to ");
  //  // Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.enableSTA(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  //  // Serial.print(".");
  //  ESP.wdtFeed();
  }
  //  // Serial.println("");
  //
 //// Serial.println("WiFi connected");
  //  // Serial.println("IP address: ");
  //  // Serial.println(WiFi.localIP());
  //  // Serial.println(WiFi.SSID());
  //
  //  // Serial.println("Starting UDP");
  udp.begin(localPort);
  //  // Serial.print("Local port: ");
  //  // Serial.println(udp.localPort());
  setTime(ntpServerName);
  while (!if_ntp) {
    delay(500);
    setTime(ntpServerName);
//    ESP.wdtFeed();
  }

  //  Serial.begin(9600);
  clock1307.begin();
//  clock1307.startClock();
  print_time(); //初始化一下变量 好改时间
  clock1307set();
//  WiFi.disconnect(WIFI_STA);
  ESP.deepSleep(2*60*60e6 );//睡眠时间2小时
}

void clock1307set() {
  clock1307.fillByYMD(y, mon, d); //Jan 19,2013
  clock1307.fillByHMS(h, m, s);   //15:28 30"

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
void loop() {
  printTime();
  print_time();  setTime(ntpServerName);
    clock1307set();
 // ESP.wdtFeed();
  delay(1000);
}
//打印ESP8266 RTC 时间
void print_time() {
  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);

  now = time(nullptr);
  //// Serial.print(now.yr, now.mon, now.date, now.hr, now.min, now.sec);

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
  //// Serial.print("DATE: ");
//  // Serial.print(buf);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
void setTime(const char *ntpServerName) {
  //get a random server from the pool
 // WiFi.hostByName(ntpServerName, timeServerIP);
 // digitalWrite(LED_BUILTIN, LOW);
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(200);

  int cb = udp.parsePacket();
  if (!cb) {
  //  // Serial.println("no packet yet");
    if_ntp = false;
  } else {

    //    // Serial.print("packet received, length=");
    //    // Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //   // Serial.print("Seconds since Jan 1 1900 = ");
    //   // Serial.println(secsSince1900);
    //    // now convert NTP time into everyday time:
    //   // Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    //  // Serial.println(epoch);

    time_t rtc = epoch;
    timeval tv = { rtc, 0 };
    timezone tz = { TZ_MN + DST_MN, 0 };
    settimeofday(&tv, &tz);
    if_ntp = true;
   // digitalWrite(LED_BUILTIN, HIGH);
    //    // print the hour, minute and second:
    //    // Serial.print("The UTC time is "); // UTC is the time at Greenwich Meridian (GMT)
    //    // Serial.print((epoch % 86400L) / 3600); // print the hour (86400 equals secs per day)
    //    // Serial.print(':');
    //    if (((epoch % 3600) / 60) < 10) {
    //      // In the first 10 minutes of each hour, we'll want a leading '0'
    //      // Serial.print('0');
    //    }
    //    // Serial.print((epoch % 3600) / 60); // print the minute (3600 equals secs per minute)
    //    // Serial.print(':');
    //    if ((epoch % 60) < 10) {
    //      // In the first 10 seconds of each minute, we'll want a leading '0'
    //      // Serial.print('0');
    //    }
    //    // Serial.println(epoch % 60); // print the second
  }
  // wait ten seconds before asking for the time again
}

//打印DS1307 RTC 时间
void printTime() {
  clock1307.getTime();

  /*// Serial.print(clock1307.hour, DEC);
  // Serial.print(":");
  // Serial.print(clock1307.minute, DEC);
  // Serial.print(":");
  // Serial.print(clock1307.second, DEC);
  // Serial.print("  ");
  // Serial.print(clock1307.month, DEC);
  // Serial.print("/");
  // Serial.print(clock1307.dayOfMonth, DEC);
  // Serial.print("/");
  // Serial.print(clock1307.year + 2000, DEC);
  // Serial.print(" ");
  // Serial.print(clock1307.dayOfMonth);
  // Serial.print("*");*/
  switch (clock1307.dayOfWeek) // Friendly printout the weekday
  {
  case MON:
    // Serial.print("MON");
    break;
  case TUE:
    // Serial.print("TUE");
    break;
  case WED:
    // Serial.print("WED");
    break;
  case THU:
    // Serial.print("THU");
    break;
  case FRI:
    // Serial.print("FRI");
    break;
  case SAT:
    // Serial.print("SAT");
    break;
  case SUN:
    // Serial.print("SUN");
    break;
  }
  //// Serial.println(" ");
}
