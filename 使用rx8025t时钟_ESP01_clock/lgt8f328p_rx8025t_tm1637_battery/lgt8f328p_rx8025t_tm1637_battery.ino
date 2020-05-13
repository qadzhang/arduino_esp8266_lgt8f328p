//使用的库  ; Grove 4  Digit Display;
// RX8025T 库 https://github.com/qadzhang/RX8025_RTC_ArduinoLibrary
//从RX8025T读RTC时间 显示到数码管
//建议使用lg8f328p 这个国产兼容芯，因为这货3.3V也是16MHZ 电压范围是5.5-3V 啥也不用管，直接用18650电池供电就行
/*
 Power consumption @ 5v
 Clock   Pro mini style w/o power LED  Pro mini style  Nano style
 32MHz  12.7mA  15.0mA  32.6mA
 16MHz  9.2mA   11.5mA  27.8mA
 8MHz   7.1mA   9.4mA   25.4mA
 4MHz   5.9mA   8.2mA   23.3mA
 2MHz   5.3mA   7.6mA   23.4mA
 1MHz   5.0mA   7.3mA   22.8mA

 */

/*******************************************************************************/
//#include  <TimerOne.h>
#include <TM1637.h>
#define ON 1
#define OFF 0

#include <TimeLib.h>
#include <Wire.h>
#include "RX8025_RTC.h"
#define SECOND_ADJUSTMENT 16
//#include <WDT.h>
#include <PMU.h>

int8_t TimeDisp[] = { 0x00, 0x00, 0x00, 0x00 };
volatile unsigned char ClockPoint = 1;
volatile long en_esp = 0; //esp01 使能
//unsigned char Update= ON;
//unsigned char halfsecond = 0;
//unsigned char second;
unsigned char minutes = 0;
unsigned char hours = 12;

RX8025_RTC rtc;
tmElements_t tm;

unsigned int esp_time = 60 * 1; //1分钟 esp01通电时间
unsigned long re_esp_time = 12 * 60 * 60 * 1; //ESP01循环时间 3小时

//#define CLK 6 //pins definitions for TM1637 and can be changed to other ports
//#define DIO 7

#define CLK 10 //pins definitions for TM1637 and can be changed to other ports
#define DIO 9
TM1637 tm1637(CLK, DIO);

void setup() {
//Serial.begin(9600);
	Wire.begin();
	pinMode(7, OUTPUT);
//  digitalWrite(7, HIGH);

	// delay to check wdt reset condition
	delay(1000);
	/*
	 #if defined(__LGT8FX8P__)
	 wdt_enable(WTO_2S);
	 #else
	 wdt_enable(WTO_512MS);
	 #endif
	 */

	uint32_t guid = (GUID3 << 24) | (GUID2 << 16) | (GUID1 << 8) | GUID0; // 给guid赋值唯一ID
	//Serial.println(guid); // 串口输出唯一ID
	tm1637.set(BRIGHT_DARKEST); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

	tm1637.init();
	//initDateTime();
// rtc.write(tm);
	//Timer1.initialize(2*500000);         //timing for 500ms
	// Timer1.attachInterrupt(TimingISR); //declare the interrupt serve routine:TimingISR
	tm1637.display(TimeDisp);
	ADCSRA = 0; // disable ADC
	// #define ADCSRA _SFR_MEM8(0x7A) 再启用的话就是这个值

}

void loop() {
	if (en_esp == 0) {
		digitalWrite(7, HIGH);
	}
	if (en_esp == esp_time) {
		digitalWrite(7, LOW);
	}
	// put your main code here, to run repeatedly:
	//wdt_reset();
	//digitalToggle(0);
	en_esp++;
	if (en_esp == re_esp_time) {
		en_esp = 0;
	}
//  delay(1);
//  if (Update == ON) {
	if (en_esp > 4) { //ESP01使能不能离太远，否则有干扰，会导致使能失败！！！ 或者考虑并联电容 或者缩短导线距离！！！！！！
		TimingISR();
		TimeUpdate();
		tm1637.display(TimeDisp);
	}

	// }

	/*
	 *休眠模式 功能说明
	 空闲模式(IDLE) 仅仅关闭内核时钟，其他外设模块正常工作，所有有效中断源均可 以将内核唤醒
	 省电模式(Save) 与 DPS0 模式相同，Save 模式为与 LGT8FX8D 保持兼容
	 掉电模式(DPS0) 与 Save 模式相同，支持唤醒源包括：
	  所有引脚电平变化
	  看门狗定时唤醒
	  异步模式的 TMR2 唤醒
	 掉电模式(DPS1) 关闭所有内外部振荡器, 支持唤醒源包括：
	  所有引脚外部电平变化
	  外部中断 0/1
	  工作于 32K LFRC 的看门狗定时器
	 掉电模式(DPS2) 关闭内核电源, 最低功耗模式，支持的唤醒源包括：
	  外部复位
	  PORTD 引脚电平变化
	  LPRC 定时唤醒(128ms/256ms/512ms/1s)
	 需要注意，从 DPS2 唤醒的过程与上电复位相同

	 PM_IDLE,
	 PM_POWERDOWN = 2,
	 PM_POFFS1 = 3,
	 PM_POFFS0 = 6
	 */

	//  ADCSRA = 0; // disable ADC
	PMU.sleep(PM_POWERDOWN, SLEEP_1S);
	// delay(1000);
}
void TimingISR() {

	ClockPoint = !ClockPoint;

}

bool initDateTime() {
	const char *monthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	char mon[12];
	int Year, Month, Day, Hour, Minute, Second;

	if (sscanf(__DATE__, "%s %d %d", mon, &Day, &Year) != 3) {
		return false;
	}
	if (sscanf(__TIME__, "%d:%d:%d", &Hour, &Minute, &Second) != 3) {
		return false;
	}

	uint8_t idx;
	Month = 0;
	for (idx = 0; idx < 12; idx++) {
		if (strcmp(mon, monthNames[idx]) == 0) {
			Month = idx + 1;
			break;
		}
	}
	if (Month == 0) {
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
void printTime() {
	tmElements_t tm2;
	tm2 = rtc.read();

	hours = (tm2.Hour);
// Serial.println(tm2.Hour); // 串口输出唯一ID
	minutes = (tm2.Minute);
//Serial.println(tm2.Minute); // 串口输出唯一ID
}
void TimeUpdate(void) {
	if (ClockPoint)
		tm1637.point(POINT_ON);
	else
		tm1637.point(POINT_OFF);
	printTime(); //设置数据 准备显示时间
	// Update = OFF;

	TimeDisp[0] = hours / 10;
	TimeDisp[1] = hours % 10;
	TimeDisp[2] = minutes / 10;
	TimeDisp[3] = minutes % 10;

}
