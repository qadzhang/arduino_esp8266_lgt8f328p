#include <PMU.h>
/*
 *LGT8F328P 低功耗睡眠测试。这个需要LGT8F328P自己的专有库PMU.h
 */
uint8_t sleepPin = 7;
uint8_t vTmp = 0;
volatile long g_IntFlag = 0;
volatile boolean LED_state = 1;
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

}

void loop() {
  Serial.write("hello, I am working!\n");
  delay(100);
  Serial.println(LED_state);
  delay(100);
  Serial.println(g_IntFlag);
  delay(100);
  g_IntFlag++;

  if (g_IntFlag % 2 == 0) {
    digitalWrite(LED_BUILTIN, LED_state);
    LED_state = !LED_state;
  };

  PMU.sleep(PM_POFFS0, SLEEP_4S);
}
