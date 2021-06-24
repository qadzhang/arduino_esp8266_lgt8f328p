// BMP180_I2C.ino
//
// shows how to use the BMP180MI library with the sensor connected using I2C.
//
// Copyright (c) 2018 Gregor Christandl
//
// connect the BMP180 to the Arduino like this:
// Arduino - BMC180
// 5V ------ VCC
// GND ----- GND
// SDA ----- SDA
// SCL ----- SCL

/*
 * BMP180MI
 * SimplePgSQL
 * 
 * 
 */

#include <Arduino.h>
#include <Wire.h>

#include <BMP180I2C.h>

#define I2C_ADDRESS 0x77

//create an BMP180 object using the I2C interface
BMP180I2C bmp180(I2C_ADDRESS);

float Temperature = 0;
float Pressure = 0;
String   Temperatures ;
String  Pressures ;

#ifdef ESP8266
#include <ESP8266WiFi.h>
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

IPAddress PGIP(192, 168, 1, 5); // your PostgreSQL server IP

const char ssid[] = "network_ssid"; //  your network SSID (name)
const char pass[] = "network_pass"; // your network password

const char user[] = "db_username";     // your database user
const char password[] = "db_password"; // your database password
const char dbname[] = "db_name";       // your database name
int port = 5432;                       // your database port

int WiFiStatus;
WiFiClient client;

char buffer[1024];
PGconnection conn(&client, 0, 1024, buffer);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  //wait for serial connection to open (only necessary on some boards)
  while (!Serial)
    ;

  Wire.begin();

  //begin() initializes the interface, checks the sensor ID and reads the calibration parameters.
  if (!bmp180.begin())
  {
    Serial.println("begin() failed. check your BMP180 Interface and I2C Address.");
    while (1)
      ;
  }

  //reset sensor to default parameters.
  bmp180.resetToDefaults();

  //enable ultra high resolution mode for pressure measurements
  bmp180.setSamplingMode(BMP180MI::MODE_UHR);

  WiFi.begin((char *)ssid, pass);
}

void checkConnection()
{
  int status = WiFi.status();
  if (status != WL_CONNECTED)
  {
    if (WiFiStatus == WL_CONNECTED)
    {
      Serial.println("Connection lost");
      WiFiStatus = status;
    }
  }
  else
  {
    if (WiFiStatus != WL_CONNECTED)
    {
      Serial.println("Connected");
      WiFiStatus = status;
    }
  }
}

void bmp180_measure()
{

  // put your main code here, to run repeatedly:

  //start a temperature measurement
  if (!bmp180.measureTemperature())
  {
    Serial.println("could not start temperature measurement, is a measurement already running?");
    return;
  }

  //wait for the measurement to finish. proceed as soon as hasValue() returned true.
  do
  {
    delay(100);
  } while (!bmp180.hasValue());

  Serial.print("Temperature: ");
  Temperature = bmp180.getTemperature();
  Serial.print(Temperature);

  Serial.println(" degC");

  //start a pressure measurement. pressure measurements depend on temperature measurement, you should only start a pressure
  //measurement immediately after a temperature measurement.
  if (!bmp180.measurePressure())
  {
    Serial.println("could not start perssure measurement, is a measurement already running?");
    return;
  }

  //wait for the measurement to finish. proceed as soon as hasValue() returned true.
  do
  {
    delay(100);
  } while (!bmp180.hasValue());

  Serial.print("Pressure: ");
  Pressure = bmp180.getPressure();
  Serial.print(Pressure);
  Serial.println(" Pa");

  Temperatures = (Temperature, DEC);
  Pressures = (Pressure, DEC);


}

int pg_status = 0;



void doPg()
{
  String query_sql = "insert  into esp_recodes values (now(),  inet_client_addr() ,upper('eerefcf'),'DHT','{ "" temperature "":    " +  Temperatures  + ", "" barometer "":" + Pressures   + "}')  ";

  char query_tables[query_sql.length() + 1];
  query_sql.toCharArray(query_tables, query_sql.length() + 1);
  query_sql = "";
  Serial.println(query_tables);


  char *msg;
  int rc;
  if (!pg_status)
  {
    conn.setDbLogin(PGIP,
                    user,
                    password,
                    dbname,
                    "utf8", port);
    pg_status = 1;
    return;
  }

  if (pg_status == 1)
  {
    rc = conn.status();
    if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED)
    {
      char *c = conn.getMessage();
      if (c)
        Serial.println(c);
      pg_status = -1;
    }
    else if (rc == CONNECTION_OK)
    {
      pg_status = 2;
      Serial.println("Enter query");
    }
    return;
  }
  if (pg_status == 2)
  {

    if (conn.execute(query_tables, true))
      goto error;
    Serial.println("Working...");
    pg_status = 3;
    return;
  }
  if (pg_status == 3)
  {
    rc = conn.getData();
    int i;
    if (rc < 0)
      goto error;
    if (!rc)
      return;
    if (rc & PG_RSTAT_HAVE_COLUMNS)
    {
      for (i = 0; i < conn.nfields(); i++)
      {
        if (i)
          Serial.print(" | ");
        Serial.print(conn.getColumn(i));
      }
      Serial.println("\n==========");
    }
    else if (rc & PG_RSTAT_HAVE_ROW)
    {
      for (i = 0; i < conn.nfields(); i++)
      {
        if (i)
          Serial.print(" | ");
        msg = conn.getValue(i);
        if (!msg)
          msg = (char *)"NULL";
        Serial.print(msg);
      }
      Serial.println();
    }
    else if (rc & PG_RSTAT_HAVE_SUMMARY)
    {
      Serial.print("Rows affected: ");
      Serial.println(conn.ntuples());
    }
    else if (rc & PG_RSTAT_HAVE_MESSAGE)
    {
      msg = conn.getMessage();
      if (msg)
        Serial.println(msg);
    }
    if (rc & PG_RSTAT_READY)
    {
      pg_status = 2;
      Serial.println("Enter query");
    }
  }
  return;
error:
  msg = conn.getMessage();
  if (msg)
    Serial.println(msg);
  else
    Serial.println("UNKNOWN ERROR");
  if (conn.status() == CONNECTION_BAD)
  {
    Serial.println("Connection is bad");
    pg_status = -1;
  }
}

void loop()
{

  checkConnection();
  if (WiFiStatus == WL_CONNECTED)
  {


    bmp180_measure();
    doPg();

    Serial.println("PG is bad bad");

  }

  delay(5 * 60);
}