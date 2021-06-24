/*
 * SimplePgSQL.c - Lightweight PostgreSQL connector for Arduino
 * Copyright (C) Bohdan R. Rau 2016 <ethanak@polip.com>
 *
 * SimplePgSQL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SimplePgSQL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SimplePgSQL.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

/*
 * Demo program for SimplePgSQL library
 * Simple PostgreSQL console
 * Accepts:
 * - PostgreSQL simple queries
 * - \d - displays table list
 * - \d tablename - list table columns
 * - exit - closes connection
 */

#include <stdlib.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#else

// Uncomment line below if you use WiFi shield instead of Ethernet
// #define USE_ARDUINO WIFI 1

#ifdef USE_ARDUINO_WIFI
#include <WIFI.h>
#else
#include <Ethernet.h>
#endif

#endif
#include <SimplePgSQL.h>

 IPAddress PGIP(192, 168, 1, 5);        // your PostgreSQL server IP 
//const char ssid[] = "network_ssid";      //  your network SSID (name)
//const char pass[] = "network_pass";      // your network password

const char user[] = "esp";       // your database user
const char password[] = "Espdsfsfs";   // your database password
const char dbname[] = "esp";         // your database name

#if defined(ESP8266) || defined(USE_ARDUINO_WIFI)
int WiFiStatus;
WiFiClient client;
#else
#define USE_ARDUINO_ETHERNET 1
EthernetClient client;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // your mac address
byte ip[] = {192, 168, 1, 177};                    // your IP address
#endif

char buffer[1024];
PGconnection conn(&client, 0, 1024, buffer);

void setup(void) {
	Serial.begin(
#ifdef ESP8266
			115200
#else
    9600
#endif
			);
#ifdef USE_ARDUINO_ETHERNET
    Ethernet.begin(mac, ip);
#else
	//WiFi.begin((char*) ssid, pass);
	WiFiManager wifiManager;
#endif

//	WiFiManager wifiManager;
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
	if (!wifiManager.autoConnect("ESP_AP", "changeit")) {
		Serial.println(F("Failed to connect. Reset and try again. . ."));
		delay(3000);
		//重置并重试
		ESP.reset();
		delay(5000);
	}

	//在我们从网络浏览器中选择Wi-Fi网络和密码后，我们现在连接到Wi-Fi网络：

	//如果你到这里，你已经连接到WiFi
	Serial.println(F("Connected to Wifi."));
	Serial.print(F("My IP:"));
	Serial.println(WiFi.localIP());
	String hostname = "esp8266_";
	hostname += ESP.getChipId();
	WiFi.hostname(hostname);
	ESP.wdtEnable(WDTO_4S); //打开看门狗
	//  ADD_STARTUP_OPTION_P(PSTR("application_name"), PSTR("arduino"));

	/*
	 *

	 #define FPM_SLEEP_MAX_TIME 0xFFFFFFF
	 void WiFiOn() {

	 wifi_fpm_do_wakeup();
	 wifi_fpm_close();

	 //Serial.println("Reconnecting");
	 wifi_set_opmode(STATION_MODE);
	 wifi_station_connect();
	 }


	 void WiFiOff() {

	 //Serial.println("diconnecting client and wifi");
	 //client.disconnect();
	 wifi_station_disconnect();
	 wifi_set_opmode(NULL_MODE);
	 wifi_set_sleep_type(MODEM_SLEEP_T);
	 wifi_fpm_open();
	 wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);

	 }


	 You control WiFi using wifi.setmode().

	 ESP8266's WiFi is very versatil. You can be a client and/or an access point. You can get your IP from DHCP or static.

	 Per default, ESP8266 start in STATIONAP mode (as client and access point).

	 This is the pertinent part from the official page

	 wifi.setmode()

	 Configures the WiFi mode to use. NodeMCU can run in one of four WiFi modes:

	 Station mode, where the NodeMCU device joins an existing network

	 Access point (AP) mode, where it creates its own network that others can join

	 Station + AP mode, where it both creates its own network while at the same time being joined to another existing network

	 WiFi off

	 When using the combined Station + AP mode, the same channel will be used for both networks as the radio can only listen on a single channel.

	 Note

	 WiFi configuration will be retained until changed even if device is turned off.

	 Syntax

	 wifi.setmode(mode[, save])
	 Parameters

	 mode value should be one of
	 wifi.STATION for when the device is connected to a WiFi router. This is often done to give the device access to the Internet.
	 wifi.SOFTAP for when the device is acting only as an access point. This will allow you to see the device in the list of WiFi networks (unless you hide the SSID, of course). In this mode your computer can connect to the device, creating a local area network. Unless you change the value, the NodeMCU device will be given a local IP address of 192.168.4.1 and assign your computer the next available IP address, such as 192.168.4.2.
	 wifi.STATIONAP is the combination of wifi.STATION and wifi.SOFTAP. It allows you to create a local WiFi connection and connect to another WiFi router.
	 wifi.NULLMODE changing WiFi mode to NULL_MODE will put wifi into a low power state similar to MODEM_SLEEP, provided wifi.nullmodesleep(false) has not been called.

	 save choose whether or not to save wifi mode to flash
	 true WiFi mode configuration will be retained through power cycle. (Default)
	 false WiFi mode configuration will not be retained through power cycle.
	 Returns

	 current mode after setup

	 Example

	 wifi.setmode(wifi.STATION);
	 See also

	 wifi.getmode() wifi.getdefaultmode()







	 */
	// WiFi.disconnect();
	// WiFi.mode(WIFI_OFF);
	//  WiFi.forceSleepBegin();
}

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
void WiFiOn() {

	wifi_fpm_do_wakeup();
	wifi_fpm_close();

	//Serial.println("Reconnecting");
	wifi_set_opmode(STATION_MODE);
	wifi_station_connect();
}

void WiFiOff() {

	//Serial.println("diconnecting client and wifi");
	//client.disconnect();
	wifi_station_disconnect();
	wifi_set_opmode(NULL_MODE);
	wifi_set_sleep_type(MODEM_SLEEP_T);
	wifi_fpm_open();
	wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);

}

#ifndef USE_ARDUINO_ETHERNET
void checkConnection() {
	int status = WiFi.status();
	if (status != WL_CONNECTED) {
		if (WiFiStatus == WL_CONNECTED) {
			Serial.println("Connection lost");
			WiFiStatus = status;
		}
	} else {
		if (WiFiStatus != WL_CONNECTED) {
			Serial.println("Connected");
			WiFiStatus = status;
		}
	}
}

#endif

/*
 static PROGMEM const char query_rel[] = "\
SELECT a.attname \"Column\",\
  pg_catalog.format_type(a.atttypid, a.atttypmod) \"Type\",\
  case when a.attnotnull then 'not null ' else 'null' end as \"null\",\
  (SELECT substring(pg_catalog.pg_get_expr(d.adbin, d.adrelid) for 128)\
   FROM pg_catalog.pg_attrdef d\
   WHERE d.adrelid = a.attrelid AND d.adnum = a.attnum AND a.atthasdef) \"Extras\"\
 FROM pg_catalog.pg_attribute a, pg_catalog.pg_class c\
 WHERE a.attrelid = c.oid AND c.relkind = 'r' AND\
 c.relname = %s AND\
 pg_catalog.pg_table_is_visible(c.oid)\
 AND a.attnum > 0 AND NOT a.attisdropped\
    ORDER BY a.attnum";


 static PROGMEM const char query_tables[] = "\
 select * from esp_insert limit 10;";
 */

int pg_status = 0;

void doPg(void) {

	/*
	 *
	 setDbLogin

	 int setDbLogin(IPAddress server,
	 const char *user,
	 const char *passwd = NULL,
	 const char *db = NULL,
	 const char *charset = NULL,
	 int port = 5432);

	 Initialize connection.
	 */
	char *msg;
	int rc;
	if (!pg_status) {
		conn.setDbLogin(PGIP, user, password, dbname, "utf8", 9988);
		pg_status = 1;
		Serial.println("conn.setDbLogin");
		return;
	}

	if (pg_status == 1) {
		rc = conn.status();
		if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED) {
			char *c = conn.getMessage();
			if (c)
				Serial.println(c);

			pg_status = -1;
		} else if (rc == CONNECTION_OK) {
			pg_status = 2;
			Serial.println("Enter query");
		}
		return;
	}

	/*
	 if (pg_status == 2) {
	 if (!Serial.available())
	 return;
	 char inbuf[64];
	 int n = Serial.readBytesUntil('\n', inbuf, 63);
	 while (n > 0) {
	 if (isspace(inbuf[n - 1]))
	 n--;
	 else
	 break;
	 }
	 inbuf[n] = 0;

	 if (!strcmp(inbuf, "\\d")) {
	 if (conn.execute(query_tables, true))
	 goto error;
	 Serial.println("Working...");
	 pg_status = 3;
	 return;
	 }
	 if (!strncmp(inbuf, "\\d", 2) && isspace(inbuf[2])) {
	 char *c = inbuf + 3;
	 while (*c && isspace(*c))
	 c++;
	 if (!*c) {
	 if (conn.execute(query_tables, true))
	 goto error;
	 Serial.println("Working...");
	 pg_status = 3;
	 return;
	 }
	 if (conn.executeFormat(true, query_rel, c))
	 goto error;
	 Serial.println("Working...");
	 pg_status = 3;
	 return;
	 }
	 */if (pg_status == 2) {

		// int random(int num);
		//	 random函数返回一个0~num-1之间的随机数. random(num)是在stdlib.h中的一个宏定义. num和函数返回值都是整型数.

		String sql_query =
				" insert  \
		   into esp_insert ( datenow, vaules, s_id, k_id ,up_addr,client_addr ,s_kind)\
		   select \
		   now() dateNow,  ";
		sql_query += random(1000);
		sql_query += " vaules, '001' s_iD, '";
		sql_query += ESP.getChipId();
		sql_query += "',inet_client_addr(),'";
		sql_query += WiFi.localIP().toString();
		sql_query += "','random';";

		/*
		 String str1 = String(along);
		 str1 += "mimi";
		 char cArr[str1.length() + 1];
		 char cArr2[str1.length() + 3];
		 str1.toCharArray(cArr,str1.length() + 1);
		 str1.toCharArray(cArr2,str1.length() + 3);
		 */

		char query_rel[sql_query.length() + 1];
		sql_query.toCharArray(query_rel, sql_query.length() + 1);
		/*
		 static PROGMEM const char query_rel[] =
		 " insert  \
				 		 		   into esp_insert ( datenow, vaules, s_id, k_id )\
				 		 		   select \
				 		 		   now() dateNow, 12 vaules, '001' s_iD, '\ ESP.getChipId();\";
		 */

		//if (conn.execute(query_tables, true))
		if (conn.execute(query_rel, true))
			goto error;
		Serial.println("Working...");
		pg_status = 3;
		/*
		 if (!strncmp(inbuf, "exit", 4)) {
		 conn.close();
		 Serial.println("Thank you");
		 pg_status = -1;
		 return;
		 }
		 if (conn.execute(inbuf))
		 goto error;
		 Serial.println("Working...");
		 pg_status = 3;
		 */
	}
	if (pg_status == 3) {
		rc = conn.getData();
		int i;
		if (rc < 0)
			goto error;
		if (!rc)
			return;
		if (rc & PG_RSTAT_HAVE_COLUMNS) {
			for (i = 0; i < conn.nfields(); i++) {
				if (i)
					Serial.print(" | ");
				Serial.print(conn.getColumn(i));
			}
			Serial.println("\n==========");
		} else if (rc & PG_RSTAT_HAVE_ROW) {
			for (i = 0; i < conn.nfields(); i++) {
				if (i)
					Serial.print(" | ");
				msg = conn.getValue(i);
				if (!msg)
					msg = (char*) "NULL";
				Serial.print(msg);
			}
			Serial.println();
		} else if (rc & PG_RSTAT_HAVE_SUMMARY) {
			Serial.print("Rows affected: ");
			Serial.println(conn.ntuples());
		} else if (rc & PG_RSTAT_HAVE_MESSAGE) {
			msg = conn.getMessage();
			if (msg)
				Serial.println(msg);
		}
		if (rc & PG_RSTAT_READY) {
			pg_status = 2;
			Serial.println("Enter query");
		}
	}
	return;
	error: msg = conn.getMessage();
	if (msg)
		Serial.println(msg);
	else
		Serial.println("UNKNOWN ERROR");
	if (conn.status() == CONNECTION_BAD) {
		Serial.println("Connection is bad");
		pg_status = -1;
	}
	conn.close();
}

void loop() {
#ifndef USE_ARDUINO_ETHERNET
	checkConnection();
	if (WiFiStatus == WL_CONNECTED) {
#endif
		doPg();
#ifndef USE_ARDUINO_ETHERNET
	}
#endif
	ESP.wdtFeed();
	delay(1000);
}

