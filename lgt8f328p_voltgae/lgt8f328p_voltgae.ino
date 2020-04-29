/*
 ReadAnalogVoltage

 lgt8f328p读取电压 Wemos TTGO XI 8F328P-U Board
 */
// the setup routine runs once when you press reset:
int light; //读取数值
int max = 4000; //转换数值上限
int min = 3200; //转换数值下限
void setup() {
	// initialize serial communication at 9600 bits per second:
	Serial.begin(9600);
	// analogReference(INTERNAL1V1);//use internal 1.1v as Avref
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
//type：使用哪种参考类型（DEFAULT, INTERNAL, INTERNAL1V1, INTERNAL2V56,INTERNAL4V096 或者 EXTERNAL）。
//返回
//无

	uint32_t guid = (GUID3 << 24) | (GUID2 << 16) | (GUID1 << 8) | GUID0; // 给guid赋值唯一ID
	Serial.println(guid); // 串口输出唯一ID


}

// the loop routine runs over and over again forever:
void loop() {
	// read the input on analog pin 0:
	int sensorValue = analogRead(A0);
	light = map(sensorValue, min, max, 0, 7);
	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
//  float voltage = sensorValue * (3.3 / 4064.0);
	// print out the value you read:
	Serial.print(sensorValue);
	Serial.print("  ");
	Serial.print(constrain(sensorValue, min, max));
	Serial.print("  ");
	Serial.println(light);

	delay(1000);
}
