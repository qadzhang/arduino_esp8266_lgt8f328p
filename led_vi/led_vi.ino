/*
 ReadAnalogVoltage

 lgt8f328p读取电压 Wemos TTGO XI 8F328P-U Board
 */
// the setup routine runs once when you press reset:
#include <math.h >

#include "TM1637.h"
#define CLK 10//pins definitions for TM1637 and can be changed to other ports
#define DIO 9
TM1637 tm1637(CLK, DIO);
unsigned char ClockPoint = 1;
int sensorValue = 0;

unsigned char hour = 0;
unsigned char minute = 0;
int8_t TimeDisp[] = { 0x00, 0x00, 0x00, 0x00 };

void TimeUpdate(void) {
	int sensorValuesa = cbrt(sensorValue) * 100; //取立方根
	//int sensorValue1 = constrain(sensorValue - 0.5 * 4096 / 2.56, 850, 2500);

//	int sensorValuesa = log10(sensorValue1) * 1000; //以10为底的对数

	int voltage = sensorValue * (2.56 / 4096.0) * 100;
	//int BRIGHTS = constrain(sensorValuesa, 2800, 3200);
//	int BRIGHT = map(BRIGHTS, 2800, 3200, 0, 7);

	int BRIGHTS = constrain(sensorValuesa, 1000, 1320);
	int BRIGHT = map(BRIGHTS, 1000, 1320, 0, 7);

	if (ClockPoint) {
		tm1637.point(POINT_ON);
		Serial.println(sensorValuesa / 1);
		delay(100);
		tm1637.set(BRIGHT);
		TimeDisp[0] = sensorValuesa / 100 / 10;
		TimeDisp[1] = sensorValuesa / 100 % 10;
		TimeDisp[2] = sensorValuesa % 100 / 10;
		TimeDisp[3] = sensorValuesa % 100 % 10;
	} else {
		tm1637.point(POINT_OFF);
		Serial.println(voltage / 1);
		delay(100);
		tm1637.set(BRIGHT);
		TimeDisp[0] = 0;
		TimeDisp[1] = 0;
		TimeDisp[2] = 0;
		TimeDisp[3] = BRIGHT;
	}
	Serial.println(BRIGHT);
	delay(100);
	ClockPoint = !ClockPoint;
	//int sensorValuesa = cbrt(sensorValue) * 100;
	//TimeDisp[0,1,2,3] = sensorValuesa /  1;

}

void setup() {
	// initialize serial communication at 9600 bits per second:
	Serial.begin(9600);

	tm1637.init();
	tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

	analogReference(INTERNAL2V56); //use internal 1.1v as Avref
	//analogReference(EXTERNAL); //使用外部基准

//analogReference(type)
//描述
//配置用于模拟输入的基准电压（即输入范围的最大值）。选项有:
//DEFAULT：默认5V（Ocrobot控制板为5V）或3.3伏特（Ocrobot控制板为3.3V）为基准电压。
//INTERNAL：在ATmega168和ATmega328上以1.1V为基准电压，以及在ATmega8上以2.56V为基准电压（Ocrobot Mega无此选项）
////INTERNAL1V1：以1.1V为基准电压（此选项仅针对Ocrobot Mega）
//INTERNAL2V56：以2.56V为基准电压（此选项仅针对Ocrobot Mega）
//EXTERNAL：以AREF引脚（0至5V）的电压作为基准电压。
//参数
//type：使用哪种参考类型（DEFAULT, INTERNAL, INTERNAL1V1, INTERNAL2V56, INTERNAL4V096 或者 EXTERNAL）。
//返回
//无

	uint32_t guid = (GUID3 << 24) | (GUID2 << 16) | (GUID1 << 8) | GUID0; // 给guid赋值唯一ID
	Serial.println(guid); // 串口输出唯一ID
	delay(2000);
}

// the loop routine runs over and over again forever:
void loop() {
	// read the input on analog pin 0:
	sensorValue = analogRead(A0);
	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (2.56 / 4096.0);
	// print out the value you read:
	Serial.print(sensorValue);
	Serial.print(' ');
	Serial.print(cbrt(sensorValue));
	Serial.print(' ');
	Serial.println(voltage);

	delay(1000);
	TimeUpdate();
	tm1637.display(TimeDisp);
}
