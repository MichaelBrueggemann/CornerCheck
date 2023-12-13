#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <time.h>
#include "custom_wifi_init.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// MAC adress
uint8_t myAddress[] = {0x4D, 0x61, 0x72, 0x74, 0x69, 0x02};

typedef struct data
{
  int decibel;
  // time_t measurement_time;
  int64_t sending_time;
  int64_t latency;

} data;

// struct to store incoming data
data stationMsg;
// array to to store the data of each sensor in
data sensors[3];

// storage for raw time
struct timeval tv_now;

// NTP Server for time synchronization
const char *ntpServer = "pool.ntp.org";

void onMessageReceived(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
  memcpy(&stationMsg, incomingData, sizeof(stationMsg));
  sensors[macAddr[5]].decibel = stationMsg.decibel;
  gettimeofday(&tv_now, NULL);
  int64_t seconds = (int64_t)tv_now.tv_sec;
  sensors[macAddr[5]].latency = (seconds - stationMsg.sending_time);
}

Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();

void setup()
{

  Serial.begin(115200);

  // estatblish wifi connection
  initUniWiFi("uni-ms");

  Serial.println("synchronizing NTP Server");
  // time server synchronization
  // ACHTUNG GEHT 1 STUNDE FALSCH
  configTime(1, 0, ntpServer);

  // wait some time to ensure synchronization
  delay(10000);
  Serial.println("NTP Server synchronized");

  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, myAddress);

  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init success");
  }
  else
  {
    Serial.println("ESPNow Init fail");
    return;
  }

  esp_now_register_recv_cb(onMessageReceived);

  matrix.begin(0x70);
}

// The different smileys
static const uint8_t PROGMEM
    smile_bmp[] =
        {B00111100,
         B01000010,
         B10100101,
         B10000001,
         B10100101,
         B10011001,
         B01000010,
         B00111100},
    neutral_bmp[] =
        {B00111100,
         B01000010,
         B10100101,
         B10000001,
         B10111101,
         B10000001,
         B01000010,
         B00111100},
    angry_bmp[] =
        {B00111100,
         B01000010,
         B10100101,
         B10000001,
         B10011001,
         B10100101,
         B01000010,
         B00111100};

void loop()
{
  // fetch current time
  // Serial.print("Current seconds since 01.01.1970 ");
  // gettimeofday(&tv_now, NULL);
  // int64_t seconds = (int64_t)tv_now.tv_sec;
  // Serial.print(seconds);
  // Serial.print("\n");
  // delay(5000);

  for (int i = 0; i < 3; i++)
  {
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.println(":");
    Serial.print("Decibel [dB]: ");
    Serial.println(sensors[i].decibel);
    Serial.print("Latency [s]: ");
    Serial.println(sensors[i].latency);
    Serial.println();
  }
  Serial.println();

  for (int i = 0; i < 3; i++)
  {
    sensors[i].latency = 0;
    sensors[i].decibel = 0;
  }

  // visulaize emojis
  if (db < 30)
  {
    matrix.clear();
    matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_GREEN);
    matrix.writeDisplay();
    delay(100)
  };
  else if (db < 45)
  {
    matrix.clear();
    matrix.drawBitmap(0, 0, neutral_bmp, 8, 8, LED_YELLOW);
    matrix.writeDisplay();
    delay(100);
  }
  else
  {
    matrix.clear();
    matrix.drawBitmap(0, 0, angry_bmp, 8, 8, LED_RED);
    matrix.writeDisplay();
    delay(100);
  }

  delay(5000);
}