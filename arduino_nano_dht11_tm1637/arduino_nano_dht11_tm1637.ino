
//使用的库 timerone; Grove 4  Digit Display; SimpleDHT;
//从DHT11读温湿度 显示到数码管
//建议使用lg8f328p 这个国产兼容芯，因为这货3.3V也是16MHZ 电压范围是5.5-3V 啥也不用管，直接用18650电池供电就行 不用非得5V或者3.3V要求供电

//
/*******************************************************************************/
#include  <TimerOne.h>

#include <TM1637.h>
#define ON 1
#define OFF 0
#include <SimpleDHT.h>

int8_t TimeDisp[] = { 0x00, 0x00, 0x00, 0x00 };
unsigned char ClockPoint = 1;
unsigned char Update;
unsigned char halfsecond = 0;
unsigned char second;
unsigned char minute = 0;
unsigned char hour = 12;

int pinDHT11 = 11;
SimpleDHT11 dht11(pinDHT11);

#define CLK 6 //pins definitions for TM1637 and can be changed to other ports
#define DIO 7
TM1637 tm1637(CLK, DIO);

void setup() {
	tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

	tm1637.init();
	Timer1.initialize(500000);         //timing for 500ms
	Timer1.attachInterrupt(TimingISR); //declare the interrupt serve routine:TimingISR
}
void readDht11() {
	byte temperature = 0;
	byte humidity = 0;
	int err = SimpleDHTErrSuccess;
	if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) { // @suppress("Ambiguous problem")
		//  Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
		return;
	}
	hour = (int) temperature;
	minute = (int) humidity;
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
	readDht11();
	Update = OFF;
}
