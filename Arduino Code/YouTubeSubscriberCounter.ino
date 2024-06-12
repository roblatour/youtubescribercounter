// YouTube Subscriber Counter - version 1.1 - updated to support AMOLED V2
//                            - version 1 - initial release
//
// Copyright Rob Latour, 2023
// htts://raltour.com/youtubesubscribercounter
// https://github.com/roblatour/youtubesubscribercounter
//
// Compile and upload using Arduino IDE (2.1.1 or greater)
//
// Physical board:                 LILYGO T-Display-S3 AMOLED
// Board in Arduino board manager: ESP32S3 Dev Module
//
// Arduino Tools settings:
// USB CDC On Boot:                Enabled
// CPU Frequency:                  240MHz (WiFi)
// Core Debug Level:               None
// USB DFU On Boot:                Enabled (Requires USB-OTG Mode)
// Erase All Flash Before Upload:  Disabled
// Events Run On:                  Core 1
// Flash Mode:                     QIO 80Mhz
// Flash Size:                     16MB (128Mb)
// ITAG Adapter:                   Disabled
// Arduino Runs On:                Core 1
// USB Firmware MSC On Boot:       Disabled
// Partition Scheme:               16 M Flash (3MB APP/9.9MB FATFS)
// PSRAM:                          OPI PSRAM
// USB Mode:                       Hardware CDC and JTAG
// Upload Mode:                    UART0 / Hardware CDC
// Upload Speed:                   921600
// Programmer                      ESPTool

// WIFI connection
#include <WiFi.h>

//OTA updates
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>
#include "rm67162.h"
#include "NotoSansBold24.h"
#include "NotoSansBold36.h"
#include "NotoSansBold72.h"
#include "YouTube_graphic.h"

#include "general_settings.h"
#include "secret_settings.h"

const char* WiFiSSID = SECRET_SETTINGS_WIFI_SSID;
const char* WiFiPassword = SECRET_SETTINGS_WIFI_PASSWORD;

#include <EEPROM.h>
const int eepromDataStartingAddress = 0;
const int numberOfSavedSettingsInEEPROM = 2;
const String confirmEEPROMHasBeenIntialize = "EEPROM initialized";
const int eepromSettingsStartingAddressForSettings = confirmEEPROMHasBeenIntialize.length() + 1;
const int eepromMaxSize = eepromSettingsStartingAddressForSettings + numberOfSavedSettingsInEEPROM + 1;

String programNameAndVersion = "YouTube Subscriber Counter v1";

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define topButtonIfUSBIsOnTheRight 21
#define bottomButtonIfUSBIsOnTheRight 0
#define PIN_LED_v1_Display_v2 38  // GPIO 38 is used to turn on/off the Green LED on the AMOLED V1 and to turn on/off the Display on the AMOLED v2  

#define PIN_IN1 46
#define PIN_IN2 45

bool USBOnTheRight;
int displayOption;

const int maxDisplayOptions = 7;

bool ledON = false;

unsigned short foreground = TFT_WHITE;
unsigned short background = TFT_BLACK;

bool saveSettingsInTenSeconds = false;
unsigned long tenSecondsFromNow;

int currentRotation;

unsigned long nextUpdate = 0;

String DevicesIPAddress;

void setupEEPROM() {

  // The EEPROM is used to save and store settings so that they are retained when the device is powered off and restored when it is powered back on
  // data is stored in the EEPROM as follows:
  //
  //      starting at eepromInitializationConfirmationAddress:
  //
  //         a string with the value of "EEPROM initialized" (without the quotes)
  //
  //         a byte; used to store if the LILYGO T-Display-S3 AMOLED is positioned such that the USB is on the right (values are true and false)
  //
  //         a byte; used to store the display option (values are 1 through 7) - see the file general_settings.h for more information

  EEPROM.begin(eepromMaxSize);

  bool eepromHasBeenInitialize = true;

  // test that the EEPROM has been initialized; if it has not been initialized then do that now
  for (int i = 0; i < confirmEEPROMHasBeenIntialize.length(); i++)
    if (EEPROM.read(i + eepromDataStartingAddress) != confirmEEPROMHasBeenIntialize[i])
      eepromHasBeenInitialize = false;

  if (!eepromHasBeenInitialize)
    initializeEEPROM();

  LoadSettingsFromNonVolatileMemory();
};

void initializeEEPROM() {

  Serial.println("Initializing EEPROM");

  for (int i = 0; i < confirmEEPROMHasBeenIntialize.length(); i++)
    EEPROM.write(eepromDataStartingAddress + i, confirmEEPROMHasBeenIntialize[i]);

  for (int i = confirmEEPROMHasBeenIntialize.length(); i < eepromMaxSize; i++)
    EEPROM.write(eepromDataStartingAddress + i, 0);

  loadDefaultSettings();
  SaveSettingsInNonVolatileMemory();

  EEPROM.commit();
  delay(1000);
}

void loadDefaultSettings() {

  USBOnTheRight = GENERAL_SETTINGS_USB_ON_THE_RIGHT;
  displayOption = GENERAL_SETTINGS_DISPLAY_OPTION;
};

void SaveSettingsInNonVolatileMemory() {

  Serial.println("Saving settings");

  byte Setting[numberOfSavedSettingsInEEPROM];

  if (USBOnTheRight) {
    Setting[0] = 1;
    Serial.println(" USB on the right");
  } else {
    Setting[0] = 0;
    Serial.println(" USB on the left");
  };

  Setting[1] = displayOption;

  Serial.print("   Display option: ");
  Serial.println(Setting[1]);

  // write the settings to their respective eeprom storage locations only if they need updating

  bool commitRequired = false;

  for (int i = 0; i < numberOfSavedSettingsInEEPROM; i++) {
    if (EEPROM.read(eepromSettingsStartingAddressForSettings + i) != Setting[i]) {
      EEPROM.write(eepromSettingsStartingAddressForSettings + i, Setting[i]);
      commitRequired = true;
    };
  };

  if (commitRequired) {
    EEPROM.commit();
    delay(1000);
  };
};

void LoadSettingsFromNonVolatileMemory() {

  Serial.println("Loading settings");

  byte Setting[numberOfSavedSettingsInEEPROM];

  for (int i = 0; i < numberOfSavedSettingsInEEPROM; i++)
    Setting[i] = EEPROM.read(eepromSettingsStartingAddressForSettings + i);

  if (Setting[0] == 1) {
    USBOnTheRight = true;
    Serial.println(" USB on the right");
  } else {
    USBOnTheRight = false;
    Serial.println(" USB on the left");
  };

  if ((Setting[1] == 0) || (Setting[1] >= maxDisplayOptions))
    displayOption = 1;
  else
    displayOption = Setting[1];

  Serial.print(" Display option: ");
  Serial.println(displayOption);
}

bool checkButtons() {

  // This routine returns true if a button was pushed, otherwise it returns false

  // The top button is used to change the display orientation and the display options
  // The bottom button is used to trigger an api update request in the next loop cycle

  // If the top button is pressed for less than 5 seconds, the display option is changed when the button is released
  // If the top button is pressed for 5 seconds or more, the orientation of the display is changed when the button is released

  // Additionally, if the top button is pushed, a flag will be set so that the changes made will be saved ten seconds now
  // The reason for the delay is to save writes to non-volatile memory as the user could potentially continue to push the top button many times to scroll through
  // to the display options until they find one they like
  // Accordingly only after the top button hasn't been used in over ten seconds will the non-volatile memory be updated

  // if the bottom button is pressed, when it is released nextUpdate will be set to zero,
  // this will force an apu update request the next loop cycle

  int topButton;
  int bottomButton;

  // On the LILYGO T-Display-S3 AMOLED the postion of the top and bottom buttons are relative to the side on which the USB cable is plugged into
  // The code below adjusts for that

  if (USBOnTheRight) {
    topButton = topButtonIfUSBIsOnTheRight;
    bottomButton = bottomButtonIfUSBIsOnTheRight;
  } else {
    topButton = bottomButtonIfUSBIsOnTheRight;
    bottomButton = topButtonIfUSBIsOnTheRight;
  };

  bool returnValue = false;

  // top button logic

  if (digitalRead(topButton) == 0) {

    returnValue = true;

    const unsigned long fiveSeconds = 5 * 1000;
    const unsigned long tenSeconds = 10 * 1000;

    unsigned long startTimer = millis();

    // hold here for up to five seconds or until the button is pushed (which ever comes first)
    while (digitalRead(topButton) == 0) {
      if ((millis() - startTimer) >= fiveSeconds)
        break;
      else
        delay(50);
    };

    unsigned long endTimer = millis();

    if ((endTimer - startTimer) < fiveSeconds) {

      displayOption++;
      if (displayOption > maxDisplayOptions)
        displayOption = 1;

    } else {

      USBOnTheRight = !USBOnTheRight;
      setupDisplayOrientation();
    };

    // trigger saving the settings to non-volatile memory
    saveSettingsInTenSeconds = true;
    tenSecondsFromNow = millis() + tenSeconds;
  };

  // bottom button logic

  if (digitalRead(bottomButton) == 0) {

    if (GENERAL_SETTINGS_GO_LIVE) {

      returnValue = true;

      // light the green led while the button is pressed so that the user knows that the program is paying attention
      if (GENERAL_SETTINGS_AMOLED_VERSION == 1)
        Set_Green_LED_v1_or_Display_v2(true);
      // hold here until the bottom button is released
      while (digitalRead(bottomButton) == 0)
        delay(50);
      if (GENERAL_SETTINGS_AMOLED_VERSION == 1)
        Set_Green_LED_v1_or_Display_v2(false);
      nextUpdate = 0;
    };
  };

  return returnValue;
}

void setupDisplayOrientation() {

  // Important Note: as the TFT will be rotated either 90 or 270 degrees the TFT Width will in used for
  // height calculations and the TFT Height will in used for width calculations through out this sketch

  if (USBOnTheRight)
    currentRotation = 1;  // rotate 90 degrees
  else
    currentRotation = 3;
  lcd_setRotation(currentRotation);  // rotage 270 degrees
};

void draw(int &subscriberCount, int &viewCount) {

  // code to not refresh the screen if not needed

  sprite.fillSprite(background);

  switch (displayOption) {
    case 1:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the subscriber count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_RED);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(subscriberCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 2);
      sprite.unloadFont();
      // show the word 'Subscribers'
      sprite.loadFont(NotoSansBold24);
      sprite.drawString("Subscribers", YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH - 20);
      break;
    case 2:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the subscriber count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_RED);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(subscriberCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 2);
      break;
    case 3:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the view count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_WHITE);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(viewCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 2);
      sprite.unloadFont();
      // show the word 'Views'
      sprite.loadFont(NotoSansBold24);
      sprite.drawString("Views", YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH - 20);
      break;
    case 4:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the subscriber count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_WHITE);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(viewCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 2);
      break;
    case 5:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the subscriber and view count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_RED);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(subscriberCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 4);
      sprite.setTextColor(TFT_WHITE);
      sprite.drawString(formatYouTubeNumber(viewCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, (TFT_WIDTH / 4) * 3 - 15);
      sprite.unloadFont();
      // show the words 'Subscribers' and 'Views'
      sprite.loadFont(NotoSansBold24);
      sprite.setTextColor(TFT_RED);
      sprite.drawString("Subscribers", YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 4 + 45);
      sprite.setTextColor(TFT_WHITE);
      sprite.drawString("Views", YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, (TFT_WIDTH / 4) * 3 + 35);
      break;
    case 6:
      // draw the Youtube graphic
      sprite.pushImage(0, 49, YouTubeGraphicWidth, YouTubeGraphicHeight, Youtube_graphic);
      lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
      // show the subscriber and view count
      sprite.loadFont(NotoSansBold72);
      sprite.setTextColor(TFT_RED);
      sprite.setTextDatum(MC_DATUM);
      sprite.drawString(formatYouTubeNumber(subscriberCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, TFT_WIDTH / 4 + 20);
      sprite.setTextColor(TFT_WHITE);
      sprite.drawString(formatYouTubeNumber(viewCount), YouTubeGraphicWidth + (TFT_HEIGHT - YouTubeGraphicWidth) / 2, (TFT_WIDTH / 4) * 3 - 10);
      break;
    case 7:
      // show the about window
      sprite.loadFont(NotoSansBold24);
      sprite.setTextColor(TFT_WHITE);
      sprite.setTextDatum(TL_DATUM);
      const int quarterHeight = (TFT_WIDTH / 4);  // note: TFT Width is used in this calculation as the screen is rotated 90 degrees
      const int horizontalOffset = 25;

      sprite.drawString(programNameAndVersion, 0, horizontalOffset);

      sprite.drawString("Copyright 2023, Rob Latour", 0, quarterHeight + horizontalOffset);

      String connectedIPAddress = "Device's IP address: ";
      connectedIPAddress.concat(DevicesIPAddress);
      sprite.drawString("rlatour.com/youtubesubscribercounter", 0, quarterHeight * 2 + horizontalOffset);

      sprite.setTextColor(TFT_LIGHTGREY);
      sprite.drawString(connectedIPAddress, 0, quarterHeight * 3 + horizontalOffset);
      break;
  };

  lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
  sprite.unloadFont();
}

void setupWiFi() {

  const int leftBoarder = 0;
  const int horizontalOffset = 20;
  const int thirdHeight = (TFT_WIDTH / 3);

  String message;

  sprite.fillSprite(background);

  if (GENERAL_SETTINGS_SHOW_WIFI_CONNECTING_STATUS) {

    message = "Connecting to ";
    message.concat(WiFiSSID);

    sprite.loadFont(NotoSansBold36);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString(message, leftBoarder, horizontalOffset);
    lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
  };

  Serial.println("Attempting to connect ...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFiSSID, WiFiPassword);

  Serial.println("Checking for connection ...");

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  };

  Serial.println("WiFi connected");

  Serial.println("WiFi setting up OTA");

  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  //Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(SECRET_SETTINGS_OTA_DEVICE_NAME);

  // No authentication by default
  ArduinoOTA.setPassword(SECRET_SETTINGS_OTA_PASSWORD);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  DevicesIPAddress = WiFi.localIP().toString().c_str();

  if (GENERAL_SETTINGS_SHOW_WIFI_CONNECTING_STATUS) {

    sprite.setTextColor(TFT_GREEN);
    sprite.drawString("Connected!", leftBoarder, thirdHeight + horizontalOffset);

    message = "IP address: ";
    message.concat(DevicesIPAddress);
    sprite.setTextColor(TFT_LIGHTGREY);
    sprite.drawString(message, leftBoarder, thirdHeight * 2 + horizontalOffset);

    lcd_PushColors(0, 0, TFT_HEIGHT, TFT_WIDTH, (uint16_t *)sprite.getPointer());
    sprite.unloadFont();

    delay(5000);
  };
};

bool getYoutubeSubscriberCount(int &subscriberCount, int &viewCount) {

  HTTPClient http;

  bool returnValue = false;

  String url = "https://youtube.googleapis.com/youtube/v3/channels?part=statistics&id=";
  url += SECRET_SETTINGS_YOUTUBE_CHANEL_ID;
  url += "&fields=items/statistics/subscriberCount,items/statistics/viewCount&key=";
  url += SECRET_SETTINGS_GOOGLE_API;

  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {

    String response = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    // Check for errors in deserialization
    if (error) {
      Serial.println(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      http.end();
    } else {

      JsonObject root = doc.as<JsonObject>();

      // Check if the JSON response contains the expected keys
      if (root.containsKey("items") && root["items"].as<JsonArray>().size() > 0 && root["items"][0].as<JsonObject>().containsKey("statistics") && root["items"][0]["statistics"].as<JsonObject>().containsKey("subscriberCount")) {

        subscriberCount = root["items"][0]["statistics"]["subscriberCount"].as<int>();
        viewCount = root["items"][0]["statistics"]["viewCount"].as<int>();

        Serial.print("Subscriber count = ");
        Serial.print(subscriberCount);
        Serial.print("; view count = ");
        Serial.println(viewCount);

        http.end();
        returnValue = true;
      } else {
        Serial.println("JSON response did not contain expected keys");
        http.end();
      }
    }
  } else {
    Serial.print("Error on sending request: ");
    Serial.println(httpResponseCode);
    http.end();
  };

  return returnValue;
}

String convertFloatToStringWithOneDecimalPlace(float x) {

  char tmp[10];
  dtostrf(x, 3, 1, tmp);
  return (String)tmp;
}

String formatYouTubeNumber(int x) {

  // returns "n" where x < a thousand
  // returns "n.n K" where x < a million; always rounding down, for example: 9,999 = "9.9 K"
  // returns "n.n M" where x < a trillion; always rounding down, for example: 9,999,999 = "9.9 M"
  // returns "n.n T" where x >= a trillion; always rounding down, for example: 9,999,999,9999 = "9.9 T"

  String returnValue;

  if (x < 1000) {
    returnValue = String(x);
  } else {
    if (x < 1000000) {
      returnValue = convertFloatToStringWithOneDecimalPlace((float)(int(x / 100) * 0.1));
      returnValue += " K";
    } else {
      if (x < 1000000000) {
        returnValue = convertFloatToStringWithOneDecimalPlace((float)(int(x / 100000) * 0.1));
        returnValue += " M";
      } else {
        returnValue = convertFloatToStringWithOneDecimalPlace((float)(int(x / 100000000) * 0.1));
        returnValue += " T";
      };
    };
  };

  return returnValue;
}

void Set_Green_LED_v1_or_Display_v2(bool toggleOn) {

  // GPIO 38 is used to turn on/off the Green LED on the AMOLED V1 and to turn on/off the Display on the AMOLED v2 

  if (toggleOn) {
    digitalWrite(PIN_LED_v1_Display_v2, HIGH);
  } else {
    digitalWrite(PIN_LED_v1_Display_v2, LOW);
  }
}

void setup() {

  Serial.begin(GENERAL_SETTINGS_SERIAL_MONITOR_SPEED);
  Serial.println(programNameAndVersion);

  pinMode(topButtonIfUSBIsOnTheRight, INPUT_PULLUP);
  pinMode(bottomButtonIfUSBIsOnTheRight, INPUT_PULLUP);

  pinMode(PIN_LED_v1_Display_v2, OUTPUT);
  if (GENERAL_SETTINGS_AMOLED_VERSION == 1) {
    // turn off the green LED
    Set_Green_LED_v1_or_Display_v2(false);
  } else {
    // turn on the LCD display
    Set_Green_LED_v1_or_Display_v2(true);
  };

  setupEEPROM();

  sprite.createSprite(TFT_HEIGHT, TFT_WIDTH);
  sprite.setSwapBytes(1);

  rm67162_init();

  setupDisplayOrientation();

  setupWiFi();

  if (!GENERAL_SETTINGS_GO_LIVE) {
    int subscriberCount = 0;
    int viewCount = 0;
    draw(subscriberCount, viewCount);
  };
}

void loop() {

  static int subscriberCount = 0;
  static int viewCount = 0;

  // The following are used to save changes setting to non-volatile memory
  // this is done ten seconds following the last setting change

  if (GENERAL_SETTINGS_GO_LIVE) {

    if (millis() > nextUpdate) {

      nextUpdate = millis() + GENERAL_SETTINGS_MINUTES_BETWEEN_UPDATES * 60 * 1000;

      if (getYoutubeSubscriberCount(subscriberCount, viewCount)) {
        draw(subscriberCount, viewCount);
      } else {
        Serial.print("Problem getting subscriber count");
      };
    };
  };

  if (checkButtons())
    draw(subscriberCount, viewCount);

  if ((saveSettingsInTenSeconds) && (millis() > tenSecondsFromNow)) {
    saveSettingsInTenSeconds = false;

    // the settings will only be saved if the display option is less then the maxDisplayOptions value
    // this as it is unlikely the user wants their saved display option to be the 'about' screen (displayOption == maxDisplayOptions)

    if (displayOption < maxDisplayOptions) {
      if (GENERAL_SETTINGS_AMOLED_VERSION == 1)
        Set_Green_LED_v1_or_Display_v2(true);
      SaveSettingsInNonVolatileMemory();
      delay(500);
      if (GENERAL_SETTINGS_AMOLED_VERSION == 1)
        Set_Green_LED_v1_or_Display_v2(false);
    };
  };

  ArduinoOTA.handle();
}
