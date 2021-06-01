#include <Arduino.h>
#include <AWS_IOT.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include "EmonLib.h"
#include <driver/adc.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// The GPIO pin were the CT sensor is connected to.
#define ADC_INPUT 34

// The voltage of the place that will be measured. In Indonesia it usually 220V.
#define HOME_VOLTAGE 220.0

// Force EmonLib to use 10bit ADC resolution
#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)

/* Add lcd display configuration */
const int lcdColumns = 16;
const int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

/* Add energy monitor configuration */
EnergyMonitor emon;

/* Wifi Credential */
const char *WIFI_SSID = "mia2lite";
const char *WIFI_PASSWORD = "babybanana";

/* Add AWS configuration */
char HOST_ADDRESS[]="a2k4y0lq1dzhua-ats.iot.us-east-1.amazonaws.com";
char CLIENT_ID[]= "HomeEnergyMonitor1";
char PUB_TOPIC_NAME[]= "arn:aws:iot:us-east-1:987152363815:thing/HomeEnergyMonitor1";

AWS_IOT hornbill;
int connectionStatus = WL_IDLE_STATUS;
char sendPayload[512];

/* Store data to send to AWS */
short measurements[30];
unsigned long firstTimeStamp = 0;
short measureIndex = 0;
unsigned long lastMeasurement = 0;
unsigned long timeFinishedSetup = 0;

/* Datetime config */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String timeStr;
unsigned long timeStamp;

void setup() {
  initialSetup();
  setupLcdDisplay();
  setupEnergyMonitor();
  connectToWiFi();
  connectToAWS();
  setupDatetime();
}

void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  if (measureIndex == 0) firstTimeStamp = timeClient.getEpochTime();
  
  unsigned long currentMillis = millis();
  
  // If it's been longer then 1000ms since we took a measurement, take one now!
  if (currentMillis - lastMeasurement > 1000) {
    double amps = emon.calcIrms(1480); // Calculate Irms only
    double watt = amps * HOME_VOLTAGE;

    lastMeasurement = millis();

    // Readings are unstable the first 5 seconds when the device powers on
    // so ignore them until they stabilise.
    if(millis() - timeFinishedSetup < 10000){
      setMessage("Startup mode", "", 0);
    } else {
      // Update the measurement
      writeEnergyToDisplay(watt);
      measurements[measureIndex] = watt;
      measureIndex++;
    }
  }

  // When we have 30 measurements, send them to AWS!
  if (measureIndex == 30) {
    setMessage("Uploading data", "to AWS...", 0);
    
    // Construct the JSON to send to AWS
    String msg = "";
    msg += "{\"readings\": [";

    for (short i = 0; i <= 28; i++){
      msg += measurements[i];
      msg += ",";
    }

    msg += measurements[29];
    msg += "]}";

    Serial.println(msg);
    msg.toCharArray(sendPayload, msg.length()+1);
    Serial.println(sendPayload);
    hornbill.publish(PUB_TOPIC_NAME,sendPayload);
    measureIndex = 0;
  }
}

void initialSetup() {
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  analogReadResolution(10);
  Serial.begin(115200);
}

void setupLcdDisplay() {
  lcd.init();                    
  lcd.backlight();
}

void setupEnergyMonitor() {
  emon.current(ADC_INPUT, 68);
  timeFinishedSetup = millis();
}

void connectToWiFi() {

  WiFi.disconnect(true);

  // Only try 12 times to connect to the WiFi
  int retries = 0;
  String wifiText = String("Wifi");
  while (connectionStatus != WL_CONNECTED && retries < 12) {
    wifiText += ".";
    setMessage("Connecting to", wifiText, 1500);
    connectionStatus = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    retries++;
  }

  // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
  if (connectionStatus != WL_CONNECTED) {
    setMessage("Wifi connection", "error!", 1000);
    esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    esp_deep_sleep_start();
  } 
  else {
    setMessage("Connected to", "Wifi!", 1000);
  }
}

void connectToAWS() {
  setMessage("Connecting to", "AWS", 1500);
  if(hornbill.connect(HOST_ADDRESS,CLIENT_ID) == 0) {
    setMessage("Connected to", "AWS", 2000);
  }
  else {
    setMessage("AWS connection", "failed", 0);
    Serial.println("AWS connection failed, Check the HOST Address");
    while(1);
  }
}

void setupDatetime() {
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  timeClient.setUpdateInterval(60000);
}

void writeEnergyToDisplay(double watts) {
  String formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  String timeStr = formattedDate.substring(splitT+1, formattedDate.length()-1);
  String wattsStr = String((int)watts) + String("W");
  Serial.println(wattsStr);
  setMessage(timeStr, wattsStr, 0);
}

void setMessage(String row1Text, String row2Text, int delayMs) {
  lcd.clear();
  if (row1Text.length() > 0) {
    lcd.setCursor(0, 0);
    lcd.print(row1Text);
  }
  if (row2Text.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(row2Text);
  }
  delay(delayMs);
}
