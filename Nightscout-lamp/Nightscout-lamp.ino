
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "time.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

//basic settings

const char *ssid = "YOUR_WIFI_SSID";   //your wifi SSID
const char *password = "YOUR_WIFI_PASSWORD";   //your wifi password
const char *TZ_INFO = "AEST-10AEDT,M10.1.0,M4.1.0/3"; // your timezone - https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char *url = "https://YOUR_NIGHTSCOUT.com/api/v1/entries?count=1&token=YOUR_TOKEN";  //your nightscout URL / Token.
#define RGBPIN 1      //Pin to which LED is attached.
#define numberLEDs 1  //number of leds attached

//Extra settings

//brightness adjustment
#define dayStart 7          //set day brightness at 7AM
#define dayEnd 20           //set night brightness at 10PM
#define dayBrightness 50    //0 - 100%
#define nightBrightness 15  //0 - 100%

//thresholds and colours
const uint8_t lowColour[] = { 0, 0, 255 };               //RGB values for low colour
#define low 3.9                                          //point at which low colour is set, above this lower in range is set
const uint8_t lowerInRangeColour[] = { 178, 102, 255 };  //RGB values for lower in range colour
#define inRange 4.8                                      // point at which in range colour is set
const uint8_t inRangeColour[] = { 0, 255, 0 };           //RGB values for in range colour
#define higherInRange 8                                  // point at which higher in range is set
const uint8_t higherInRangeColour[] = { 255, 153, 0 };   //RGB values for higher in range colour
#define high 10                                          //point at which high is set
const uint8_t highColour[] = { 255, 0, 0 };              //RGB values for high colour

//mute settings
#define muteTime 0                          // if mute time (in milliseconds - i.e. 300000 for 5 minutes) is > 0 mute pin is enabled, when mute pin is grounded i.e. via a button press the led is set to the mute colour for the defined time.
#define mutePin 0                               // pin used for mute button, pin must be a pulled up. - disabled by default.
const uint8_t muteColour[] = { 255, 200, 180 };  //mute colour - warm white by default.
#define muteBrightness nightBrightness           //custom mute brightness, set to night brightness by default.

Adafruit_NeoPixel bglPixel(numberLEDs, RGBPIN, NEO_GRB + NEO_KHZ800);

void updatePixelColour(const uint8_t colourToSet[]) {
  bglPixel.fill(bglPixel.Color(colourToSet[0], colourToSet[1], colourToSet[2]), 0, 0);
}

long updateSGV(time_t now)  //get sgv from nightscout
{
  Serial.println("attempting to get latest BGL from Nightscout");
  static JsonDocument doc;
  static int sgvDelta = -1;
  static float sgv = 0;

  static long lastReading = 0;
  long nextReading = 300000;  //by default attempt to get a reading every 5 minutes, if a reading is not recieved this value is used to retry.

  updatePixelColour((uint8_t[]){0,0,0});// pixel off by default, used in case of error scenario. 
  HTTPClient http;
  http.setReuse(false);
  http.begin(url);
  http.addHeader("accept", "application/json");
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (!error) {
      sgv = (float)doc[0]["sgv"] / 18;
      lastReading = (long long)doc[0]["date"] / 1000;
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  http.end();

  sgvDelta = (now - lastReading);         // how long ago last reading was recieved by nightscout in seconds?
  if (sgvDelta >= 0 && sgvDelta < 330) {  // if last reading recived less than 5.5 minutes ago everything normal
                                          //reading expected at least every 5.5 minutes, subtract how long ago last reading recieved from 5.5 minutes to know when next reading should arrive. 5.5 minutes is being used to give a bit of overhead to allow for delays in xDrip / AAPS / juggulloco writing to nightScout.
    nextReading = (330 - sgvDelta) * 1000;
  }
  if (sgvDelta < 900) {  // if last reading less than 15 minutes, if more than 15 minutes turn off led to avoid displaying stale data.
    //neopixel control
    if (sgv >= 1) {
      if (sgv <= low) {
        updatePixelColour(lowColour);
      } else if (sgv > low && sgv < inRange) {
        updatePixelColour(lowerInRangeColour);
      } else if (sgv >= inRange && sgv < higherInRange) {
        updatePixelColour(inRangeColour);
      } else if (sgv >= higherInRange && sgv < high) {
        updatePixelColour(higherInRangeColour);
      } else if (sgv >= high) {
        updatePixelColour(highColour);
      };
    }
  }
  bglPixel.show();
  Serial.printf("Last reading %f - delta %d - next reading expected in %d \n", sgv, sgvDelta, (nextReading / 1000));
  return nextReading;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  updatePixelColour((uint8_t[]){0,0,0});//pixel off
  bglPixel.show();
}

void setup() {
  Serial.begin(115200);
  bglPixel.begin();

  bglPixel.setBrightness(((float)dayBrightness / 100) * 255);
  updatePixelColour((uint8_t[]){0,0,0});//pixel off
  bglPixel.show();

  if (muteTime) {
    pinMode(mutePin, INPUT_PULLUP);
  }

  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("wifi Connected");
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", TZ_INFO, 1);
  tzset();
  uint8_t count = 0;
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && count < 1000) {  //wait for NTP time sync allow 1000 retries, i.e. 10 seconds max.
    Serial.println("Time not synced with NTP - retry");
    count++;
    delay(100);
  }
  (count == 1000) ? Serial.println("Time sync fail") : Serial.println("Time sync complete");
}


void loop() {
  static time_t now;
  static struct tm *locality;
  static unsigned long previousMillisSGV, currentMillis, muteStart = 0;
  static bool day = 1;
  static bool muted = 0;
  time(&now);                  // read the current time
  locality = localtime(&now);  // update the structure tm with the current time

  static long whenNext = updateSGV(now);  //get inital reading.

  currentMillis = millis();

  if (!digitalRead(mutePin) && !muted) {  // if mute button pressed and not currently muted,
    Serial.println("Muting");
    bglPixel.setBrightness(((float)muteBrightness / 100) * 255);             // set mute brightness
    updatePixelColour(muteColour); // set mute colour
    bglPixel.show();                                                         // update pixel
    muted = true;                                                            // is muted
    muteStart = millis();                                                    // when did mute start
  }

  if (muted && currentMillis - muteStart > muteTime) {  //if currently muted and has been muted for > mute time cancel mute. 
    Serial.println("Mute end - getting new reading");
    muted = false;
    (day) ? bglPixel.setBrightness(((float)dayBrightness / 100) * 255) : bglPixel.setBrightness(((float)nightBrightness / 100) * 255); //reset brightness
    whenNext = updateSGV(now);//update led and determine when to get next reading.
  }

  if (!muted) { //if not muted
    if (currentMillis - previousMillisSGV > whenNext) { //if time between now and when next reading is expected has elapsed try and get new reading.
      Serial.println("getting new reading");
      whenNext = updateSGV(now); // get time when next reading expected and update led colour.
      previousMillisSGV = currentMillis; //update when last reading retrived. 
    }
    if (locality->tm_hour >= dayStart && locality->tm_hour < dayEnd && !day) { //if during daylight hours apply day brightness
      bglPixel.setBrightness(((float)dayBrightness / 100) * 255);
      day = 1;
    } else if (locality->tm_hour >= dayEnd || locality->tm_hour < dayStart && day) { //if during evening hours apply night brightness
      bglPixel.setBrightness(((float)nightBrightness / 100) * 255);
      day = 0;
    }
  }
}
