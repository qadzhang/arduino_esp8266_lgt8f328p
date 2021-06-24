//使用的库 timerone; Grove 4  Digit Display; Grove - RTC DS1307;
//从DS1307读RTC时间 显示到数码管
//建议使用lg8f328p 这个国产兼容芯，因为这货3.3V也是16MHZ 电压范围是5.5-3V 啥也不用管，直接用18650电池供电就行
/*******************************************************************************/
#include  <TimerOne.h>

#include <TM1637.h>
#define ON 1
#define OFF 0
#include <DS1307.h>

int8_t TimeDisp[] = { 0x00, 0x00, 0x00, 0x00 };
unsigned char ClockPoint = 1;
unsigned char Update;
unsigned char halfsecond = 0;
unsigned char second;
unsigned char minute = 0;
unsigned char hour = 12;

#define CLK 6 //pins definitions for TM1637 and can be changed to other ports
#define DIO 7
TM1637 tm1637(CLK, DIO);
DS1307 clock1307; //define a object of DS1307 class

void setup() {
	tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

	tm1637.init();
	Timer1.initialize(500000);         //timing for 500ms
	Timer1.attachInterrupt(TimingISR); //declare the interrupt serve routine:TimingISR
	clock1307.begin();
	clock1307.startClock();

}

void loop() {

	if (Update == ON) {
		TimeUpdate();
		tm1637.display(TimeDisp);
	}
	delay(250);
}
void TimingISR() {
	halfsecond++;
	Update = ON;

	// Serial.println(second);
	ClockPoint = !ClockPoint;

	if (halfsecond == 2) {
		halfsecond = 0;

	}
}
void TimeUpdate(void) {
	if (ClockPoint)
		tm1637.point(POINT_ON);
	else
		tm1637.point(POINT_OFF);
	TimeDisp[0] = hour / 10;
	TimeDisp[1] = hour % 10;
	TimeDisp[2] = minute / 10;
	TimeDisp[3] = minute % 10;
	printTime(); //设置数据 准备显示时间
	Update = OFF;
}

void printTime() {
	clock1307.getTime();
//	Serial.print(clock1307.hour, DEC);
	hour = clock1307.hour;
//	Serial.print(":");
//	Serial.print(clock1307.minute, DEC);
	minute = clock1307.minute;
//	Serial.print(":");
//	Serial.print(clock1307.second, DEC);
//	Serial.print("	");
//	Serial.print(clock1307.month, DEC);
//	Serial.print("/");
//	Serial.print(clock1307.dayOfMonth, DEC);
//	Serial.print("/");
//	Serial.print(clock1307.year + 2000, DEC);
//	Serial.print(" ");
//	Serial.print(clock1307.dayOfMonth);
//	Serial.print("*");
//	switch (clock1307.dayOfWeek) // Friendly printout the weekday
//	{
//	case MON:
//		Serial.print("MON");
//		break;
//	case TUE:
//		Serial.print("TUE");
//		break;
//	case WED:
//		Serial.print("WED");
//		break;
//	case THU:
//		Serial.print("THU");
//		break;
//	case FRI:
//		Serial.print("FRI");
//		break;
//	case SAT:
//		Serial.print("SAT");
//		break;
//	case SUN:
//		Serial.print("SUN");
//		break;
//	}
//	Serial.println(" ");
}
