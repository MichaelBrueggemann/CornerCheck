#include <math.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <stdlib.h>
#include "custom_wifi_init.h"

// MAC adresses
//uint8_t receiverAddress[] = {0x4D, 0x61, 0x72, 0x74, 0x69, 0x02};
uint8_t receiverAddress[] = {0x4E, 0xA1, 0x72, 0x74, 0x69, 0x02};
uint8_t myAddress[] = {0x54, 0x69, 0x6D, 0x0A, 0x00, 0x00};

// Information of the device to connect to
esp_now_peer_info_t peerInfo;

// data that will be send
typedef struct data {
  int decibel;
  int64_t sending_time;
  int64_t latency; // will be empty and ist only used in the receiver
} data;

// storage for the message to sent
data myMessage; 

// storage for raw time
struct timeval tv_now;

// NTP Server for time synchronization
const char* ntpServer = "pool.ntp.org";

#define SoundSensorPin 3  // this pin read the analog voltage from the sound level meter
#define VREF  5.0 // voltage on AREF pin,default:operating voltage

const int measurementInterval = 125; // in ms
const int sendingInterval = 1000; // in ms

unsigned long previousMillis = 0; // last time, the sensor was read
float dbaSum = 0; // Sum of the dBA-values per sendingInterval
int readingCount = 0; // Count readings per sendingInterval
int sendingCounter = 1; // Count sendings

void messageSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  if(status == ESP_NOW_SEND_SUCCESS){
    Serial.println("Success");
  }
  else{
    Serial.println("Error");
  }
}


void setup(){
  Serial.begin(115200);
  delay(3000);

  analogReadResolution(13);

  // intialize RNG
  srand(1);

  // estatblish wifi connection
  initUniWiFi("uni-ms");
  //initHomeWifi(""); // for testing

  Serial.println("synchronizing NTP Server");
  // time server synchronization
  // ACHTUNG GEHT 1 STUNDE FALSCH
  configTime(1, 0, ntpServer);

  // wait some time to ensure synchronization
  delay(10000);
  Serial.println("NTP Server synchronized");

  WiFi.mode(WIFI_STA);
  Serial.print("Old ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(WIFI_IF_STA, myAddress);
  Serial.print("New ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());  
  
  // test initialisation
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init success");
  }
  else {
    Serial.println("ESPNow Init fail");
    return;
  }
  
  esp_now_register_send_cb(messageSent);   

  // set options for connection to peer device
  memcpy(peerInfo.peer_addr, receiverAddress, 6); // deep-copy cause of array immutability
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // try to add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop(){
  unsigned long currentMillis = millis(); // current time in ms
  // check if measurementInterval expired
  if (currentMillis - previousMillis >= measurementInterval) 
  {
    float voltageValue,dbaValue;
    voltageValue = analogReadMilliVolts(SoundSensorPin) / 1000.0; // measure Voltage of Sensor
    dbaValue = voltageValue * 50.0;  //convert voltage to decibel value
    //print measurements for testing:
    //Serial.print(dbaValue,1);
    //Serial.println(" dBA");

    // update global variables:
    dbaSum += pow(10,(dbaValue / 10.0));
    readingCount++;
    previousMillis = currentMillis;
  }

  // check if sendingInterval expired
  if (currentMillis >= sendingInterval * sendingCounter) 
  {
    // Calculate average DBA-Value and cast into int for efficient communication
    // ATTENTION: For better accuracy the value is multiplicated by 10 before the cast
    int averageDbaValueM10 = int(10 * 10 * log10(dbaSum / readingCount));
    //Serial.println("Average dBA-Value in last " + String(sendingInterval) + " ms: " + String(averageDbaValueM10));
    
    // safe dBA-Value in message
    myMessage.decibel = averageDbaValueM10;

    // get current time in seconds
    gettimeofday(&tv_now, NULL);
    int64_t seconds = (int64_t)tv_now.tv_sec;

    myMessage.sending_time = seconds;
    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myMessage, sizeof(myMessage));
    if (result != ESP_OK) {
        Serial.println("Sending error");
    }
    Serial.println("Message send: ");
    Serial.print("Decibel [dB]: ");
    Serial.println(myMessage.decibel);
    Serial.print("Sending time [s sind 01.01.1970]: ");
    Serial.println(myMessage.sending_time);

    sendingCounter++;
    dbaSum=0;
    readingCount=0;
  }
}