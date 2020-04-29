//********************************
//20190629修改：1、有wifi配置但连不上时不再自动进入smartconfig，如果需要可以在连接wifi时按住flash键，直到出现Wconfig；2、增加httpupdate，可以直接通过web更新固件；
//********************************
#include <Arduino.h>
#include <EEPROM.h>
#include <LedControl.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>
#include <pgmspace.h>
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

//定义GPIO
#define MX_CLOCK        2    // NodeMCU-D4   黑   CLK
#define MX_CS           14   // NodeMCU-D5   白   CS
#define MX_DIN          12   // NodeMCU-D6   灰   DataIn
#define KEY 			0   //按键，GPIO0 NodeMCU Flash Key
#define LED				16	//NodeMCU板载指示灯
// I2C For ESP8266, these default to SDA = GPIO04(NodeMCU-D2) and SCL = GPIO05(NodeMCU-D1).
#define LEDON LOW //低电平点亮
#define LEDOFF HIGH //高电平熄灭

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

LedControl lc=LedControl(MX_DIN, MX_CLOCK, MX_CS, 4);
unsigned long lastdisp = millis();    //LED屏刷新标志
unsigned char bn = 0;   // 显示缓冲区加载指针变量；
byte clear_buf[32] = {byte(0)};
byte disp[32] = {byte(0)};    //显示缓冲区；
byte disp_buf[32] = {byte(0)};    //待显缓冲区；
unsigned int k = 0;   //用于“:”闪烁
unsigned int t = 0;   //用于切换日期时间显示


RtcDS3231<TwoWire> Rtc(Wire);

char ssid[30], pass[30], dev_mac[18]; //  WIFI SSID/Password/Mac
static boolean wifiConfiged = false, wifiConnected = false;
#define EEPROM_NUM (sizeof(ssid)+sizeof(pass))
#define LOCATION_WIFI 0x01			//WiFiconfig 写入起始地址

const unsigned long NtpInterval = 3600*1000UL;   //NTP自动校时间隔(毫秒)
unsigned long lastNtpupdate = millis();    //NTP校时标志
boolean rtcstatus = false;   //检测RTC是否有效
boolean ntpstatus = false;   //检测NTP同步状态
unsigned int keys = 0;    //按键检测结果
IPAddress timeServerIP; // 用于存放解析后的NTP server IP；
const char* ntpServerName = "cn.ntp.org.cn";    //NTP服务器域名：尽量不要直接填写IP，


//-----------------------------------数字字符表 
unsigned char dp[][8]{
 // 5*8字模
{0x7E,0x81,0x81,0x81,0x7E,'z'},      // -0-  字符0
{0x00,0x41,0xFF,0x01,0x00,'z'},      // -1-  字符1
{0x43,0x85,0x89,0x91,0x61,'z'},      // -2-  字符2
{0x82,0x81,0x91,0xA9,0xC6,'z'},      // -3-  字符3
{0x18,0x28,0x48,0xFF,0x08,'z'},      // -4-  字符4
{0xF2,0x91,0x91,0x91,0x8E,'z'},      // -5-  字符5
{0x7E,0x91,0x91,0x91,0x4E,'z'},      // -6-  字符6
{0x80,0x8F,0x90,0xA0,0xC0,'z'},      // -7-  字符7
{0x6E,0x91,0x91,0x91,0x6E,'z'},      // -8-  字符8
{0x62,0x91,0x91,0x91,0x7E,'z'},      // -9-  字符9
{0x66,0x66,'z'},                     // -10- ":"时间分隔符
{0x18,0x18,0x18,'z'},                // -11- "-"日期分隔符
{0xC0,0xC0,0x3C,0x66,0x42,0x42,0x24,'z'},  // -12- "oC"温度符号
{0x00,0x00,'z'},                     // -13-  2空列
{0x06,0x06,'z'},                     // -14-  "."小数点
{0x01,0x00,'z'},                     // -15-  首列带点，用于NTP校时不成功时提醒
};

byte connwifi[32]{        //开机显示Connwifi；
0x7C,0x82,0x82,0x82,0x44,0x00,  // -C-
0x1C,0x22,0x22,0x22,0x1C,0x00,  // -o-
0x1E,0x20,0x20,0x1E,0x00,       // -n-
0x7E,0x04,0x18,0x04,0x7E,0x00,  // -W-
0x5E,0x00,                      // -i-
0x7E,0x50,0x50,0x50,0x00,       // -F-
0x5E,0x00,                      // -i-
};

byte wconfig[32]{        //smartConfig显示WConfig；
0x7E,0x04,0x18,0x04,0x7E,0x00,  // -W-
0x1C,0x22,0x22,0x10,0x00,  		// -c-
0x1C,0x22,0x22,0x22,0x1C,0x00,  // -o-
0x1E,0x20,0x20,0x1E,0x00,       // -n-
0x7E,0x50,0x50,0x00,     		// -F-
0x5E,0x00,                      // -i-
0x19,0x25,0x25,0x1E,			// -g-
};

unsigned long ntptime(IPAddress& address){

	const unsigned int localPort = 2395;      // 本地UDP监听端口
	const unsigned long century = 3155673600UL;        //1900--2000的秒数
	const long timeZoneOffset = +28800;     //时区GMT +8
	const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
	byte packetBuffer[NTP_PACKET_SIZE];    //buffer to hold incoming and outgoing packets
	memset(packetBuffer, 0, NTP_PACKET_SIZE);    // set all bytes in the buffer to 0

	WiFiUDP udp;    // A UDP instance to let us send and receive packets over UDP
    udp.begin(localPort);
	delay(500);
  //初始化NTP请求头；
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	udp.beginPacket(address, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
    delay(500);      // wait to see if a reply is available
    int cb = udp.parsePacket();
    if (!cb) {
        return 0; //没有收到NTP回包返回0
    }
    else {
        // received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        
        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;     //得到NTP时间（自1900年以来的秒数）
		return secsSince1900 - century + timeZoneOffset;	//返回NTP同步后的时间
    }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u/%02u/%02u %02u:%02u:%02u"),
            dt.Year(),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void scankey()
{
	if (digitalRead(KEY) == 0){
		unsigned int Keytime = 0;
		while(!digitalRead(KEY)){
			Keytime ++;
			delay(10);
		}
		if (Keytime >= 300) keys = 2;
		if (Keytime >= 2 & Keytime < 300) keys = 1;
	}
}

void putin_buf (unsigned char u){   //将字符装入显示缓冲区
  
  unsigned char a = 0;  //定义变量用于数据提取指针
  do{   
    disp_buf[bn] = dp[u][a];  //将二维数组中的一组数据放入显示缓冲区
    a++;  //换下一个提取字节
    bn++; //换下一个显示缓冲字节
  }
  while(dp[u][a] != 'z'); //当显示数据为'z'时结束循环
  bn++;
  disp_buf[bn] = 0; //显示一列的空位，便于字符区分
}

void Load_time(void){  //时间组合与显示

  RtcDateTime Rtctime;
  if (rtcstatus){
    Rtctime = Rtc.GetDateTime();   //读取RTC日期时间数据
  }
  else{
    Rtctime = now();   //RTC不可用时，读取系统日期时间；
  }
  
  bn = 0;  //初始化填充指针
  memcpy(disp_buf,clear_buf,32);    //清显示缓冲
  if (!ntpstatus){
      putin_buf(15);    //插入带点的空列，显示NTP校时异常；
  }
  else{
      putin_buf(13);     //在前面显示两列空格，让内容居中；
  }
  if(Rtctime.Hour()/10 != 0){
    putin_buf(Rtctime.Hour()/10);   //显示小时值（十位，为0时消隐）
    putin_buf(Rtctime.Hour()%10);   //显示小时值（个位）  
  }
  else{
    putin_buf(13);   //  十位为0时，插入2列空格保持显示居中
    putin_buf(Rtctime.Hour()%10);  //显示小时值（个位）    
  }
  if(k == 1){
    putin_buf(10);    //显示冒号“:"
  }
  else{
    putin_buf(13);    //显示":"闪烁效果
  }
  if(Rtctime.Minute()/10 != 0){
    putin_buf(Rtctime.Minute()/10);   //显示分钟值（十位）
    putin_buf(Rtctime.Minute()%10);   //显示分钟值（个位）  
  }
  else{
    putin_buf(0);
    putin_buf(Rtctime.Minute()%10);   //显示分钟值（个位）    
  }
}

void Load_date(void){   //日期组合与显示

  RtcDateTime Rtctime;
  if (rtcstatus){
    Rtctime = Rtc.GetDateTime();   //读取RTC日期时间数据
  }
  else{
    Rtctime = now();    //RTC不可用时，读取系统日期时间；
  }
  
  bn = 0;   //初始化填充指针
  memcpy(disp_buf,clear_buf,32);    //清显示缓冲
  if (!ntpstatus){
      putin_buf(15);    //插入带点的空列，显示NTP校时异常；
  }
  else{
      putin_buf(13);    //在前面显示两列空格，让内容居中；
  }
  if(Rtctime.Month()/10 != 0){
    putin_buf(Rtctime.Month()/10);   //显示月份值（十位）
    putin_buf(Rtctime.Month()%10);   //显示月份值（个位）  
  }
  else{
    putin_buf(13);  //  十位为0时，插入2列空格保持显示居中
    putin_buf(Rtctime.Month()%10);  //显示月份值（个位）    
  }
  putin_buf(11);    //显示日期分割符“-"

  if(Rtctime.Day()/10 != 0){
    putin_buf(Rtctime.Day()/10);  //显示日期值（十位）
    putin_buf(Rtctime.Day()%10);  //显示日期值（个位）  
  }
  else{
    putin_buf(0);
    putin_buf(Rtctime.Day()%10);  //显示日期值（个位）    
  }
}

void Load_temp(void){   //温度组合与显示
  
  float temp = 0;
  if (rtcstatus){
    RtcTemperature Temperature = Rtc.GetTemperature();    //  读取DS3231中的温度数据
    //temp = Temperature.AsFloat();
	temp = Temperature.AsFloatDegC();
  }
  else{   //RTC不可用时，使用一个错误值代替；
    temp = 99.0;
  }
  
  bn = 0; //初始化填充指针
  memcpy(disp_buf,clear_buf,32);    //清显示缓冲区
  putin_buf(13);    //在前面显示两列空格，让内容居中；
  if((temp > 60.0)|(temp < -20.0)){
    putin_buf(11);   //超出温度范围，显示"--"；
    putin_buf(11);
  }
  else{
    putin_buf(int(temp*10)/100);     //显示温度值（十位）
    putin_buf((int(temp*10)%100)/10);//显示温度值(个位)
    putin_buf(14);                   //显示小数点“." 
    putin_buf(int(temp*10)%10);     //显示温度值小数部分
  }
  putin_buf(12);                  //显示温度符号；
}

void Display() {

  int num = 0;
  for(int address=0; address<=3; address++){    //逐个扫描四个LED屏
    for(int col=0;col<8;col++) {    //每块LED屏逐列扫描；
      lc.setColumn(address,col,disp[num]);    //使用列方式显示；
      num ++;
    }
  }
}

//ssid写入eeprom
void saveConfig(void){
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i=0; i<sizeof(ssid); i++)  {*pd = ssid[i]; pd++;}
  for (i=0; i<sizeof(pass); i++)  {*pd = pass[i]; pd++;}
  for (i = 0; i < EEPROM_NUM; i++) {EEPROM.write(i + LOCATION_WIFI, data[i]);}
  EEPROM.commit();
}

void loadConfig(void) {

  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
//  char *ssid_tmp;
//  char *pass_tmp;
  int i;
  for (i = 0; i < EEPROM_NUM; i++) {data[i] = EEPROM.read(i + LOCATION_WIFI);}
  for (i=0; i<sizeof(ssid); i++){ssid[i] = *pd; pd++;}
  for (i=0; i<sizeof(pass); i++){pass[i] = *pd; pd++;}
//  Serial.printf("SSID:%s, PASS:%s\r\n", ssid, pass);
}

void smartConfig(char* ssid_tmp, char* pass_tmp)
{
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig");
  pinMode(LED, OUTPUT);
  WiFi.beginSmartConfig();
  while (1)
  {
    Serial.print("-");
    digitalWrite(LED, LEDON);
    delay(200);
    digitalWrite(LED, LEDOFF);
    delay(200);
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      strcpy(ssid_tmp, WiFi.SSID().c_str());
      strcpy(pass_tmp, WiFi.psk().c_str());
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
	  saveConfig();
	  pinMode(LED, INPUT);
      break;
    }
  }
}

void connectWifi(char *dev_mac_tmp){
	Serial.print("Connecting to Wifi");
//    WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
//	Serial.printf("ssid: %s,pass: %s\r\n", ssid, pass);
	unsigned int count = 0;
	while ((count < 30 ) && (WiFi.status() != WL_CONNECTED)) {
      delay(500);
      Serial.print(".");
      count ++;
	}
    if(WiFi.status() == WL_CONNECTED){
		Serial.println("Done!");
		Serial.print(WiFi.SSID());
		Serial.println(" Connected!");
		Serial.print("IP addr: ");
		Serial.println(WiFi.localIP());
		Serial.print("Mac addr: ");
		strcpy(dev_mac_tmp, WiFi.macAddress().c_str());
		Serial.println(dev_mac_tmp);	
	}else{
		Serial.println("Fail!");	
	}

    
}

void init_rtc(void){
	Rtc.Begin();
	RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);   //获取固件编译时间
	if (!Rtc.IsDateTimeValid()) 
	{
		Serial.println("RTC lost confidence in the DateTime!");
		Rtc.SetDateTime(compiled);    //设定RTC时间
	}
    if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true); //尝试启动Rtc运行
	if (!Rtc.GetIsRunning() || Rtc.LastError() !=0 ) {
		rtcstatus = false;
		setTime(compiled);		//RTC异常后，设定本地时间为编译时间；
	}else{
		Rtc.Enable32kHzPin(false);
		Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
		RtcDateTime rtctime = Rtc.GetDateTime();
		setTime(rtctime);   //获取RTC时间，设置本地系统时间
		rtcstatus = true;   //设置RTC有效
		Serial.print("Load RTC Time: "); printDateTime(rtctime);
		Serial.println("\n");
	}
  }

void setup() {
  
	Serial.begin(115200);
	Serial.println();

  //初始化LED屏
	int devices=lc.getDeviceCount();    //获取LED屏的数量
	for(int address=0; address<devices; address++) {
      lc.shutdown(address,false);   //初始化LED
      lc.setIntensity(address,6);   //设置LED亮度，0-15；
      lc.clearDisplay(address);   //清屏
	}

	EEPROM.begin(512);			//初始化EEPROM
	loadConfig();					//读取wifi配置信息
	if(strlen(ssid) != 0 & strlen(pass) != 0){
		memcpy(disp, connwifi, sizeof(connwifi));   //显示ConWiFi字样；
		Display();
		connectWifi(dev_mac);			  //连接WiFi
		if (strlen(dev_mac) != 0){
			digitalWrite(LED, LEDON);
		}
		wifiConfiged = true;
	}
	else
	{
		wifiConfiged = false;
	}

	scankey();    //  按键扫描
    if(!wifiConfiged || keys == 2){
		memcpy(disp, wconfig, sizeof(wconfig));   //显示WConfig字样；
		Display();
		smartConfig(ssid, pass);
	} 

  //初始化RTC时钟，并设定本地系统时间
	init_rtc();

	MDNS.begin(ssid);
	httpUpdater.setup(&httpServer);
	httpServer.begin();
	MDNS.addService("http", "tcp", 80);

}

void loop() { 

	scankey();    //  按键扫描
	if ((lastNtpupdate != millis() / NtpInterval) | (keys == 1)) {    //依据计时或按键（短按）进入NTP更新
		keys = 0;
		WiFi.hostByName(ntpServerName, timeServerIP);     //get a random server from the pool
		unsigned long curtime = ntptime(timeServerIP);
			if(curtime ==0) {
				Serial.print("NTP update Failure!");
				ntpstatus = false;
			}
			else {
				setTime(curtime);			//写入本地时间
				Rtc.SetDateTime(curtime);		//写入RTC时间
				Serial.print("NTP update Done!");
				ntpstatus = true;
			}
		lastNtpupdate = millis() / NtpInterval;
	}

	if (millis() / 1000 != lastdisp){   //主显示程序，循环显示时间、日期、温度
		lastdisp = millis() / 1000;
		if (t <= 6){    //时间显示保持6秒
			Load_time();
		}
		else if(t <= 8){    //日期显示保持2秒
			Load_date();
		}
		/*else if(t <= 10 && rtcstatus){   //温度显示保持2秒，RTC不存在时不显示温度
			Load_temp();
		}*/
		else{
			t = 0;
		}
		t ++;
		memcpy(disp,disp_buf,sizeof(disp_buf));   //将待显缓冲区内容移入显示缓冲
		Display();
		k = 1 - k;
	}
	
	httpServer.handleClient();
}
