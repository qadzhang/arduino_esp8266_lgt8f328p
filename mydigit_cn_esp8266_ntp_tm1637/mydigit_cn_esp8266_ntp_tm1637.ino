//所需库 TM1637 

#include <ESP8266WiFi.h>
#include <time.h>
#include <TM1637Display.h>

// WiFi Config
#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "yourPASSWORD"

// NTP Config
int timezone = 7 * 3600;
int dst = 0;

// Module connection pins (Digital Pins)
#define CLK D5
#define DIO D7
#define AmPm D1
#define TEST_DELAY   2000

const uint8_t SEG_CONN[] = { SEG_A | SEG_D | SEG_E | SEG_F,                 // C
SEG_C | SEG_D | SEG_E | SEG_G,                   // o
SEG_C | SEG_E | SEG_G,                           // n
SEG_C | SEG_E | SEG_G                            // n
};

const uint8_t SEG_PASS[] = { SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,         // P
SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // A
SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
SEG_A | SEG_C | SEG_D | SEG_F | SEG_G            // S
};

const uint8_t SEG_FAIL[] = { SEG_A | SEG_E | SEG_F | SEG_G,                 // F
SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // A
SEG_F | SEG_G,                                   // I
SEG_D | SEG_F | SEG_G                            // L
};

const uint8_t SEG_DONE[] = { SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,         // d
SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
SEG_C | SEG_E | SEG_G,                           // n
SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

TM1637Display display(CLK, DIO);

String lastTimeStr;
unsigned long lastShowColon = 0;
bool showColon = false;
bool hourcheck = false;

void setup() {
  pinMode(AmPm, INPUT);
  display.setBrightness(0x0e);

  // Connect WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  display.setSegments(SEG_CONN);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  display.setSegments(SEG_PASS);
  delay(1000);

  // Get time from NTP server
  configTime(timezone, dst, "ntp.ku.ac.th", "fw.eng.ku.ac.th",
      "ilm.live.rmutt.ac.th");
  while (!time(nullptr)) {
    delay(500);
  }
  display.setSegments(SEG_DONE);
  delay(1000);
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    connect_wifi();
  }
  uint8_t hour;
  bool hourcheck;
  time_t now = time(nullptr);
  struct tm *p_tm = localtime(&now);

  hour = p_tm->tm_hour;
  if (digitalRead(AmPm) == LOW) {
    if (hour > 12) {
      hour = hour - 12;
    }
  }
  if (hour < 1) {
    hourcheck = true;
  } else {
    hourcheck = false;
  }
  if ((p_tm->tm_sec % 2) == 0) {
    display.showNumberDecEx(hour * 100 + p_tm->tm_min, 0x40, hourcheck);
  } else {
    display.showNumberDecEx(hour * 100 + p_tm->tm_min, 0x00, hourcheck);
  }

}
void connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  display.setSegments(SEG_CONN);
}
