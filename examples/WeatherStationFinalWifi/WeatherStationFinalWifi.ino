/* Copyright (C) 2018  Samuel Trassare (https://github.com/tiogaplanet)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
// This example sketch turns MiP into a weather person and provides the
// weather data to a web client.
#include <mip_esp8266.h>
#include <JsonListener.h>
#include <time.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include "OpenWeatherMapCurrent.h"

// The following three variables must be configured by the user for the sketch to work. //////////////

// Enter the SSID for your wifi network.
char* ssid = "..............";

// Enter your wifi password.
char* password = "..............";

// Provide your OpenWeatherMap API key.  See
// https://docs.thingpulse.com/how-tos/openweathermap-key/
// for more information.
const String OPEN_WEATHER_MAP_APP_ID = "your_openweathermap_api_key";


/////////////////////////////////////////////////////////////////////////////////////////////////////

// The following two variables can be configured to the user's preference. //////////////////////////

// Provide the OpenWeatherMap ID for your city.  For example, the value for Naples, Italy
// is 3172394. Charleston, South Carolina is 4574324. Newcastle upon Tyne, GB is 2641673.
const String OPEN_WEATHER_MAP_LOCATION_ID = "3172394";

// Set any hostname you desire.
char* hostname = "MiP-0x01";

/////////////////////////////////////////////////////////////////////////////////////////////////////

// No other changes need to be made.

#define HTTP_RETRIES 3

// MiP variables.
MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

// For the chest LED.
uint8_t red, green, blue = 0;
bool chestValuesWritten = false;

// Don't update the eyes if they're already solid.
bool lastUpdatedToSolid = false;
bool extinguished = false;

// Interval in which to randomly write to the eyes, indicating rain.
const long eyesRainInterval = 500;

// A longer interval for drizzle.
const long eyesDrizzleInterval = 1000;

// An even longer interval for mist.
const long eyesMistInterval = 3000;

// Store the last time MiP's eyes were written to for animation.
unsigned long previousEyesMillis = 0;

// Track MiP's position.  MiP roams while upright and reports the weather when on the kickstand.
MiPPosition lastPosition = (MiPPosition) - 1;

// The rest of these variables are for the weather station features.

// Initiate the client for the OpenWeatherMap API.
OpenWeatherMapCurrent client;

const String OPEN_WEATHER_MAP_LANGUAGE = "en";
const boolean IS_METRIC = false;

// Read the location code from SPIFFS
String locationLine;

// Store the last time OpenWeatherMap was queried.
unsigned long previousMillis = 0;

// Retrieve weather data every 15 minutes (900000 milliseconds).
const long interval = 900000;

// A place to store the data retrieved from OpenWeatherMap.
OpenWeatherMapCurrentData data;

// Set the web server port number to 80.
ESP8266WebServer server(80);

// Variable in which to store the HTTP request.
String header;

// Function prototypes for HTTP handlers.
void handleRoot();
void handleNotFound();
void handleFormSubmit();

// For form validation when searching for a new city.
boolean searchError = false;


void setup() {
  // Establish the WiFi connection.
  connectResult = mip.begin(ssid, password, hostname);

  // Connect the ESP8266 to MiP.
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  // We'll need a random number generator for a few things such as animating the eyes in conditions of rain
  // and choosing random sounds.
  randomSeed(analogRead(A0));

  // Call the 'handleRoot' function when a client requests the URI "/".
  server.on("/", handleRoot);

  // Call the 'handleFormSubmit' function when a client submits the search form.
  server.on("/search.html", handleFormSubmit);

  // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound."
  server.onNotFound(handleNotFound);

  // Start the web server.
  server.begin();

  // We'll be able to turn MiP's eyes and chest on and off with a single clap while on the kickstand and reporting the weather.
  mip.enableClapEvents();
  mip.writeClapDelay(1000);

  SPIFFS.begin();

  // Read the default weather location from SPIFFS.
  locationLine = readLocation();
  if (locationLine.length() == 0) {
    Serial1.println(F("Using default location."));
    saveLocation(OPEN_WEATHER_MAP_LOCATION_ID);
  }
  locationLine = readLocation();

  updateWeatherById(locationLine);
}


void loop() {
  // Handle OTA updates.
  ArduinoOTA.handle();

  // This block of code pulls data from OpenWeatherMap.org every 15 minutes regardless of MiP's position.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    updateWeatherById(locationLine);
    chestValuesWritten = false;
    previousMillis = currentMillis;
  }

  MiPPosition        currentPosition = mip.readPosition();

  if (currentPosition != lastPosition) {
    if (mip.isOnBack()) {
      Serial1.println(F("Position: On Back"));
      // Make a random sound and flash the chest LED.
      randomFall();
    }
    if (mip.isFaceDown()) {
      Serial1.println(F("Position: Face Down"));
      // Make a random sound and flash the chest LED.
      randomFall();
    }
    if (mip.isUpright()) {
      Serial1.println(F("Position: Upright"));
      // Set MiP into a custom roaming mode.
      mip.disableClapEvents();
      customRoamMode();
    }
    if (mip.isPickedUp()) {
      Serial1.println(F("Position: Picked Up"));
      // Stop the wheels from spinning freely and make a noise.
      mip.stop();
      mip.playSound(MIP_SOUND_MIP_WHOAH, MIP_VOLUME_1);
    }
    if (mip.isHandStanding()) {
      Serial1.println(F("Position: Hand Stand"));
      // No special handling for this position.
    }
    if (mip.isFaceDownOnTray()) {
      Serial1.println(F("Position: Face Down on Tray"));
      // Make a random sound and flash the chest LED.
      randomFall();
    }
    if (mip.isOnBackWithKickstand()) {
      Serial1.println(F("Position: On Back With Kickstand"));
      // Make MiP ready to report the weather.  Don't forget to plug MiP in at this point.
      mip.stop();
      mip.disableRadarMode();
      mip.enableClapEvents();
      mip.writeChestLED(red, green, blue);
    }
    lastPosition = currentPosition;
  }

  // MiP is on his kickstand - start reporting the weather.
  if (mip.isOnBackWithKickstand()) {
    // Listen for claps first.  If a clap is detected, toggle the boolean extinguished variable.
    while (mip.availableClapEvents() > 0) {
      uint8_t clapCount = mip.readClapEvent();
      Serial1.println("Detected " + String(clapCount) + " clap(s).");
      if (clapCount > 0 && extinguished == false) {
        Serial1.println(F("Clap detected, switching off."));
        mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
        mip.writeChestLED(0, 0, 0);
        extinguished = true;
        lastUpdatedToSolid = false;
        chestValuesWritten = false;
      } else if (clapCount > 0 && extinguished == true) {
        Serial1.println(F("Clap detected, switching on."));
        extinguished = false;
      }
    }

    if (!extinguished) {
      if (data.description.indexOf("rain") >= 0) { // Animate the eyes to indicate rain.
        // Randomly write values to MiP's eyes to indicate rain.  Writes are done once every eyesRainInterval.
        unsigned long eyesMillis = millis();
        if (eyesMillis - previousEyesMillis >= eyesRainInterval) {
          mip.writeHeadLEDs((MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2));
          previousEyesMillis = eyesMillis;
        }
        lastUpdatedToSolid = false;
      } else if (data.description.indexOf("drizzle") >= 0) {
        // Randomly write values to MiP's eyes to indicate rain.  Writes are done once every eyesRainInterval.
        unsigned long eyesMillis = millis();
        if (eyesMillis - previousEyesMillis >= eyesDrizzleInterval) {
          mip.writeHeadLEDs((MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2));
          previousEyesMillis = eyesMillis;
        }
        lastUpdatedToSolid = false;
      } else if (data.description.indexOf("mist") >= 0) {
        // Randomly write values to MiP's eyes to indicate rain.  Writes are done once every eyesRainInterval.
        unsigned long eyesMillis = millis();
        if (eyesMillis - previousEyesMillis >= eyesMistInterval) {
          mip.writeHeadLEDs((MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2));
          previousEyesMillis = eyesMillis;
        }
        lastUpdatedToSolid = false;
      } else if (!lastUpdatedToSolid) {            // If there is no rain, turn on the eyes without animation.
        mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON);
        lastUpdatedToSolid = true;
      }

      // Update the chest LED.
      if (!chestValuesWritten) {
        chestValuesWritten = updateChestLED();
      }
    }
  }

  // Listen for HTTP requests from clients.
  server.handleClient();
}


// Play a randomly selected sound when MiP falls.
void randomFall() {
  MiPSoundIndex fallSounds[6] = { MIP_SOUND_MIP_HURT, MIP_SOUND_MIP_GRUNT_1, MIP_SOUND_MIP_GRUNT_2, MIP_SOUND_MIP_GRUNT_3, MIP_SOUND_MIP_OUCH_1, MIP_SOUND_MIP_OUCH_2 };
  mip.playSound(fallSounds[rand() % 6], MIP_VOLUME_1);
  mip.writeChestLED(0xFF, 0x00, 0x00, 990, 980);
}

// A variation on my first sketch for MiP.  MiP will roam and after encountering a defined number of near obstacles within a defined
// interval, MiP shows frustration.  This function has its own loop so the main loop will be blocked until customRoamMode() returns.
void customRoamMode() {
  // Set the chest LED to a violet color.
  mip.writeChestLED(0xB6, 0x00, 0xFF);

  mip.enableRadarMode();

  // While MiP is roaming, he may get frustrated by too many obstacles.  This is the interval in which MiP calms down.
  const long cooldownInterval = 60000;

  // Each obstruction within the cool down period increases MiP's frustration.
  uint8_t frustrationLevel = 0;

  // The number of obstructions MiP can tolerate within the cool down period before expressing frustration.
  const uint8_t frustrationThreshold = 4;

  MiPRadar lastRadar = MIP_RADAR_INVALID;

  // As soon as MiP changes position, return execution to the main loop.
  while (mip.isUpright()) {
    // Start driving.
    mip.continuousDrive(16, 0);

    MiPRadar        currentRadar = mip.readRadar();

    unsigned long currentMillis = millis();

    if (currentRadar != MIP_RADAR_INVALID && lastRadar != currentRadar) {
      switch (currentRadar)
      {
        case MIP_RADAR_NONE:
          // No obstruction, continue on happily.
          break;
        case MIP_RADAR_10CM_30CM:
          // Distant obstruction detected, take evasive maneuvers.
          randomEvasion();
          mip.continuousDrive(16, 0);
          break;
        case MIP_RADAR_0CM_10CM:
          // Near obstruction detected, reset the cool down clock and increase frustration level.
          previousMillis = currentMillis;
          frustrationLevel++;
          if (frustrationLevel != frustrationThreshold) {
            randomEvasion();
            mip.continuousDrive(16, 0);
          }
          break;
      }
      lastRadar = currentRadar;
    }

    // This is it.  If MiP has exceeded its frustration threshold, have a good ol' tantrum and go back to normal.
    if (frustrationLevel >= frustrationThreshold) {
      frustration();
      previousMillis = currentMillis;
      frustrationLevel = 0;
    } else if (currentMillis - previousMillis >= cooldownInterval) { // Otherwise, if MiP has avoided near obstacles for
      previousMillis = currentMillis;                                // the last minute, reset the frustration level.
      frustrationLevel = 0;
    }

    // Listen for HTTP requests from clients while in roaming mode.
    server.handleClient();
  }
}

// This is the actual expression of frustration, called from customRoamMode().
void frustration() {

  // Set the chest LED to red.
  mip.writeChestLED(0xFF, 0x00, 0x00);

  // Make an angry noise.
  mip.beginSoundList();
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_4, 0);
  mip.addEntryToSoundList(MIP_SOUND_MOOD_ANGRY, 1000);
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_OFF, 0);
  mip.playSoundList(0);

  // Flash the eyes angrily.
  MiPHeadLEDs headLEDs;
  headLEDs.led2 = headLEDs.led3 = MIP_HEAD_LED_BLINK_FAST;
  headLEDs.led1 = headLEDs.led4 = MIP_HEAD_LED_BLINK_SLOW;
  mip.writeHeadLEDs(headLEDs);

  // Do three spins, each in a random direction for a random number of degrees at max speed, of course.
  for (uint8_t i = 0; i < 3; i++) {
    (random(0, 2)) ? mip.turnLeft(random(0, 1276), 24) : mip.turnRight(random(0, 1276), 24);
    delay(1500);
  }

  // restore the eyes to normal.
  headLEDs.led1 = headLEDs.led2 = headLEDs.led3 = headLEDs.led4 = MIP_HEAD_LED_ON;
  mip.writeHeadLEDs(headLEDs);

  // Make an "exhaustion" noise.
  mip.beginSoundList();
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_4, 0);
  mip.addEntryToSoundList(MIP_SOUND_ACTION_OUT_OF_BREATH, 0);
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_OFF, 0);
  mip.playSoundList(0);

  // Set the chest LED back to violet and get on with life.
  mip.writeChestLED(0xB6, 0x00, 0xFF);
}

// Randomly turn right or left to avoid obstructions while in the custom roaming mode.
void randomEvasion() {
  (random(0, 2) == 0) ? mip.turnLeft(90, 12) : mip.turnRight(90, 12);
  delay(500);
}

// The rest of these functions support the weather station features.

// Save the location from the search field to SPIFFS.
void saveLocation(const String location) {
  SPIFFS.remove("/location.txt");
  File saveFile = SPIFFS.open("/location.txt", "w");
  saveFile.println(location);
  saveFile.close();
}

// Read the saved location from SPIFFS.
String readLocation() {
  File saveFile = SPIFFS.open("/location.txt", "r");
  String savedLocation = saveFile.readStringUntil('\n');
  saveFile.close();

  savedLocation.trim();

  return savedLocation;
}

// Read the weather from OpenWeatherMap by city ID.
void updateWeatherById(const String cityId) {
  // If the update fails we will restore the city name.
  String holder = data.cityName;
  data.cityName = "";

  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  client.setMetric(IS_METRIC);

  uint8_t i;

  for (i = 0; i < HTTP_RETRIES ; i++) {
    if (data.cityName.length() == 0) {
      client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, cityId);
    } else {
      Serial1.println("Found data for " + data.cityName + ", " + data.country + ".");
      break;
    }
  }

  if (i == HTTP_RETRIES && data.cityName.length() == 0) {
    Serial1.println(F("Failed updating weather."));
    data.cityName = holder;
  }
}

// Read the weather from OpenWeatherMap by city name.
void updateWeatherByName(const String cityName) {
  // If the update fails we will restore the city name.
  String holder = data.cityName;
  data.cityName = "";

  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  client.setMetric(IS_METRIC);

  uint8_t i;

  for (i = 0; i < HTTP_RETRIES ; i++) {
    if (data.cityName.length() == 0) {
      client.updateCurrent(&data, OPEN_WEATHER_MAP_APP_ID, cityName);
    } else {
      Serial1.println("Found data for " + data.cityName + ".");
      break;
    }
  }

  if (i == HTTP_RETRIES && data.cityName.length() == 0) {
    Serial1.println(F("Failed updating weather."));
    data.cityName = holder;
  }
}

// Set MiP's chest to indicate the current temperature using the algorithm, not the values, from:
// https://sjackm.wordpress.com/2012/03/26/visualizing-temperature-as-color-using-an-rgb-led-a-lm35-sensor-and-arduino/
bool updateChestLED() {
  if (data.temp) {
    // Range of blue.
    if (data.temp < 32) {
      blue = 255;
    }
    else if (data.temp > 32 && data.temp <= 72) {
      blue = map(data.temp, 32, 72, 255, 0);
    }
    else if (data.temp > 72) {
      blue = 0;
    }

    // Range of green.
    if (data.temp < 32) {
      green = 0;
    }
    else if (data.temp > 32 && data.temp <= 60) {
      green = map(data.temp, 32, 60, 0, 255);
    }
    else if (data.temp > 60 && data.temp <= 80) {
      green = 255;
    }
    else if (data.temp > 80) {
      green = map(data.temp, 80, 110, 255, 0);
    }

    // Range of red.
    if (data.temp < 72) {
      red = 0;
    }
    else if (data.temp >= 72 && data.temp <= 80) {
      red = map(data.temp, 72, 80, 1, 255);
    }
    else if (data.temp > 80) {
      red = 255;
    }

    // Write it to the chest LED.
    mip.writeChestLED(red, green, blue);
  } else {
    return false;
  }

  return true;
}

// Handle calls from the web client to the web server.
void handleRoot() {
  // Send HTTP status 200 (Ok) and the page to the client.
  server.send(200, "text/html", completePage());
}

// Handle requests to search for and change the city.
void handleFormSubmit() {
  // In case the search fails.
  String lastCity = data.cityName;

  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "city") {
        // Validate the data.
        data.cityName = "";
        updateWeatherById(server.arg(i));
        if (data.cityName.length() == 0) {
          Serial1.println(F("Failed update by ID, trying by name."));
          updateWeatherByName(server.arg(i));
          if (data.cityName.length() == 0) {
            searchError = true;

            // The search failed so put the old city back for the next page display.
            data.cityName = lastCity;
          } else {
            // The update by name was successful but do the save using the city ID.
            Serial1.println("Found data for " + data.cityName + ".");
            saveLocation(data.cityId);
            Serial1.println("Saved " + readLocation());
            locationLine = data.cityId;
            searchError = false;
            if (!extinguished) {
              updateChestLED();
            }
          }
        } else {
          // The update by ID was successful so save the user's search string for the next time.
          saveLocation(server.arg(i));
          locationLine = server.arg(i);
          searchError = false;
          if (!extinguished) {
            updateChestLED();
          }
        }
      }
    }
  }

  // Send HTTP status 200 (Ok) and the page to the client.
  server.send(200, "text/html", completePage());
}

// Handle "not found" calls from the web client to the web server.
void handleNotFound() {
  // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request.
  char errorMessage[150];

  sprintf(errorMessage,
          "404: Not found.\n\nPlease use \"http://%s\" or \"http://%s.local\" to see the weather.\n         -MiP",
          WiFi.localIP().toString().c_str(), hostname);

  server.send(404, "text/plain", errorMessage);
}

// Beyond here lies the HTML served to the client from handleRoot().
String completePage() {
  String htmlOutput = "<!DOCTYPE html>\n<html>\n";
  htmlOutput += htmlHead();
  htmlOutput += htmlBody() ;
  htmlOutput += "</html>\n";

  return htmlOutput;
}

String htmlHead() {
  String head = "<head>\n";

  // Use the weather condition icon as the favicon.
  head += "<link rel=\"icon\" href=\"http://openweathermap.org/img/w/" + data.icon + ".png\">\n";

  // Refresh every 15 minutes.
  head += "<meta http-equiv=\"refresh\" content=\"900\">\n";
  head += " <meta charset=\"UTF-8\">\n";
  head += " <meta name=\"viewport\" content=\"user-scalable=no,width=device-width\" />\n";

  head += "<style>\n";
  head += "  body {background-color: #";
  // Use the weather condition icon to determine the appropriate background color.  It's easier than checking for the
  // plain language weather description.
  if (data.icon.indexOf("01") >= 0) {         // Clear sky.
    (data.icon.indexOf('d') >= 0) ? head += "065ce5" : head += "04398e";
  } else if (data.icon.indexOf("02") >= 0) {  // Few clouds.
    (data.icon.indexOf('d') >= 0) ? head += "2b64bf" : head += "17376b";
  } else if (data.icon.indexOf("03") >= 0) {  // Scattered clouds.
    (data.icon.indexOf('d') >= 0) ? head += "3c6dbc" : head += "213e6d";
  } else if (data.icon.indexOf("04") >= 0) {  // Broken clouds.
    (data.icon.indexOf('d') >= 0) ? head += "4c71ad" : head += "2f466d";
  } else if (data.icon.indexOf("09") >= 0) {  // Shower rain.
    (data.icon.indexOf('d') >= 0) ? head += "4c6284" : head += "384860";
  } else if (data.icon.indexOf("10") >= 0) {  // Rain.
    (data.icon.indexOf('d') >= 0) ? head += "43536b" : head += "313d4f";
  } else if (data.icon.indexOf("11") >= 0) {  // Thunderstorm.
    (data.icon.indexOf('d') >= 0) ? head += "485260" : head += "333a44";
  } else if (data.icon.indexOf("13") >= 0) {  // Snow.
    (data.icon.indexOf('d') >= 0) ? head += "f9fafc" : head += "666768";
  } else if (data.icon.indexOf("50") >= 0) {  // Mist.
    (data.icon.indexOf('d') >= 0) ? head += "bbbdc1" : head += "38393a";
  }
  head += ";}\n";

  // Build a "navigation bar" ...
  head += "  .navbar {overflow: hidden; background-color: #333; position: fixed; top: 0; left: 0; width: 100%; height: 40px; margin: auto; }\n";

  // ...that contains a search box for other cities...
  head += "  .search-box,.close-icon,.search-wrapper {position: relative; padding: 5px 5px;}\n";
  head += "  .search-wrapper {width: 30%; float: left;}\n";
  head += "  .search-box {width: 200px; border: 1px solid #ccc; outline: 0; border-radius: 15px;}\n";
  head += "  .search-box:focus {box-shadow: 0 0 2px 2px #b0e0ee; border: 1px solid #bebede;}\n";
  head += "  .close-icon {border:1px solid transparent; background-color: transparent; display: inline-block; vertical-align: middle; outline: 0; cursor: pointer;}\n";
  head += "  .close-icon:after {content: \"X\"; display: block; width: 15px; height: 15px; position: absolute; background-color: #FA9595; z-index:1; right: 35px; top: 0; bottom: 0; margin: auto; padding: 2px; border-radius: 50%; text-align: center; color: white; font-weight: normal; font-size: 10px; box-shadow: 0 0 2px #E50F0F; cursor: pointer;}\n";
  head += "  .search-box:not(:valid) ~ .close-icon { display: none;}\n";
  if (searchError) {
    head += "  ::placeholder {color: red; opacity: 1;}\n";
  }

  // ...and conveniently displays the user's local time.
  head += "  .menuclock {font-family: Arial, Helvetica, sans-serif; color: #ffffff; float: right; padding: 10px 5px; margin-left: 30%;}\n";

  head += "  h1 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 200%; text-align: center; line-height: 5px;}\n";
  head += "  h2 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 300%; text-align: center; line-height: 5px;}\n";
  head += "  h3 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 110%; text-align: center; line-height: 5px;}\n";
  head += "  h4 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 100%; text-align: center; line-height: 5px;}\n";
  head += "  hr {border-top: 1px solid white;}\n";
  head += "  p {color: white; font-family: Arial, Helvetica, sans-serif;}\n";
  head += "  .weather {margin: 0 auto; max-width: 350px; margin-top: 50px; border-radius: 20px; background: rgba(0, 0, 0, .5); padding: 10px;}\n";
  head += "  canvas {padding-left: 0; padding-right: 0; margin-left: auto; margin-right: auto; display: block;}\n";
  head += "  footer {color: #d26c22; text-align: center;}\n";
  head += " </style>\n";

  head += "<title>" + data.cityName + " Weather Conditions</title>\n";

  head += " </head>\n";

  return head;
}

// Bring together the major parts of the HTML body.
String htmlBody() {
  String body = "<body>\n";
  body += "<p/>\n";
  body += htmlMenuBar();
  body += "<div class=\"weather\">\n";
  body += htmlHeader();
  body += htmlWeatherData();
  body += htmlFooter();
  body += "</div>\n";
  body += "<p/>\n";
  body += "</body>\n";

  return body;
}

String htmlMenuBar() {
  String htmlMenuBar = "<div class=\"navbar\">\n";

  htmlMenuBar += "<div class=\"search-wrapper\">\n";
  htmlMenuBar += "  <form action=\"search.html\" method=\"post\">\n";
  htmlMenuBar += "    <input type=\"text\" name=\"city\" required class=\"search-box\" placeholder=\"";
  (searchError) ? htmlMenuBar += "Invalid city name or ID" : htmlMenuBar += "City name or ID";
  htmlMenuBar += "\" />\n";
  htmlMenuBar += "      <button class=\"close-icon\" type=\"reset\"></button>\n";
  htmlMenuBar += "  </form>\n";
  htmlMenuBar += "</div>\n";

  htmlMenuBar += "<div id=\"clockbox\" class=\"menuclock\"></div>\n";
  htmlMenuBar += "<script type=\"text/javascript\">\n";
  htmlMenuBar += "var tday=[\"Sunday\",\"Monday\",\"Tuesday\",\"Wednesday\",\"Thursday\",\"Friday\",\"Saturday\"];\n";
  htmlMenuBar += "var tmonth=[\"January\",\"February\",\"March\",\"April\",\"May\",\"June\",\"July\",\"August\",\"September\",\"October\",\"November\",\"December\"];\n";

  htmlMenuBar += "function GetClock(){\n";
  htmlMenuBar += "var d=new Date();\n";
  htmlMenuBar += "var nday=d.getDay(),nmonth=d.getMonth(),ndate=d.getDate(),nyear=d.getFullYear();\n";
  htmlMenuBar += "var nhour=d.getHours(),nmin=d.getMinutes();\n";
  htmlMenuBar += "if(nmin<=9) nmin=\"0\"+nmin\n";

  htmlMenuBar += "var clocktext=\"\"+tday[nday]+\", \"+ndate+\" \"+tmonth[nmonth]+\", \"+nyear+\" \"+nhour+\":\"+nmin+\"\";\n";
  htmlMenuBar += "document.getElementById('clockbox').innerHTML=clocktext;\n";
  htmlMenuBar += "}\n";

  htmlMenuBar += "GetClock();\n";
  htmlMenuBar += "setInterval(GetClock,1000);\n";
  htmlMenuBar += "</script>\n";
  htmlMenuBar += "</div>\n";

  return htmlMenuBar;
}

// The header shows just the city, main weather description and the temperature.
String htmlHeader() {
  String header = "<header>\n";
  header += "  <h1>" + data.cityName + "</h1> \n";
  header += "<h4>" + data.main + "</h4>\n";
  header += "  <h2>" + String(round(data.temp)) + "&#176;</h2> \n";
  header += "</header>\n";

  return header;
}

// The footer contains the acknowledgement link to OpenWeatherMap.
String htmlFooter() {
  String footer = "<footer>\n";
  footer += "  <a title=\"OpenWeatherMap\" href=\"https://openweathermap.org\"><img src=\"https://openweathermap.org/themes/openweathermap/assets/vendor/owm/img/logo_OpenWeatherMap_orange.svg\" alt=\"OpenWeatherMap logo\" height=\"20\"></a>\n";
  footer += "</footer>\n";

  return footer;
}

// A nicely formatted HTML rendering of the weather data.
String htmlWeatherData() {
  String htmlOutput = "<hr />";
  htmlOutput += "<h4>Weather for " + data.cityName + ", " + data.country + "</h4>\n";
  htmlOutput += "<p>\n";
  time_t time = data.observationTime;
  htmlOutput += "Observation time: " + String(ctime(&time)) + "<br>\n";
  htmlOutput += "Description: " + data.description + "<br>\n";
  htmlOutput += "IconMeteoCon: " + data.iconMeteoCon + "<br>\n";
  htmlOutput += "Temperature: " + String(round(data.temp)) + "&#176;<br>\n";
  htmlOutput += "Pressure: " + String(data.pressure) + " hPa<br>\n";
  htmlOutput += "Humidity: " + String(data.humidity) + "&#37;<br>\n";
  htmlOutput += "Temperature minimum: " + String(round(data.tempMin)) + "&#176;<br>\n";
  htmlOutput += "Temperature maximum: " + String(round(data.tempMax)) + "&#176;<br>\n";
  htmlOutput += "Wind speed: " + String(data.windSpeed) + " mph<br>\n";
  htmlOutput += "Wind degrees: " + String(data.windDeg) + "<br>\n";
  htmlOutput += "Clouds: " + String(data.clouds) + "&#37;<br>\n";
  time = data.sunrise;
  htmlOutput += "Sunrise: " + String(ctime(&time)) + "<br>\n";
  time = data.sunset;
  htmlOutput += "Sunset: " + String(ctime(&time)) + "<br>\n";
  htmlOutput += "</p>\n";
  htmlOutput += "<hr />";

  // Show the RGB values for the chest LED.
  htmlOutput += "<h3>MiP";
  if (mip.isUpright()) {
    htmlOutput += " is roaming</h3>\n";
    htmlOutput += chestHTML(0xB6, 0x00, 0xFF);
  } else if (extinguished) {
    htmlOutput += " is muted</h3>\n";
  } else if (!extinguished) {
    htmlOutput += "</h3>\n";
    htmlOutput += chestHTML(red, green, blue);
  }
  htmlOutput += "<hr />";

  return htmlOutput;
}

// This is an HTML5 canvas object used to display the color of MiP's chest LED.
String chestHTML(const uint8_t redHTML, const uint8_t greenHTML, const uint8_t blueHTML) {
  String chestHTML = "<canvas id=\"imageView\" width=\"64\" height=\"64\"></canvas>\n";

  chestHTML += "<script type=\"text/javascript\">\n";
  chestHTML += "var canvas, context, canvaso, contexto;\n";
  chestHTML += "canvaso = document.getElementById('imageView');\n";
  chestHTML += "context = canvaso.getContext('2d');\n";

  chestHTML += "context.rect(0, 0, 64, 64);\n";
  chestHTML += "context.fillStyle=\"white\";\n";
  chestHTML += "context.fill();\n";

  chestHTML += "context.strokeStyle = '#a1a2a3';\n";
  chestHTML += "context.save();\n";
  chestHTML += "context.translate(32, 32);\n";
  chestHTML += "context.scale(0.6363636363636364, 1);\n";
  chestHTML += "context.beginPath();\n";
  chestHTML += "context.arc(0, 0, 37, 0, 6.283185307179586, false);\n";
  chestHTML += "context.fillStyle = '#";
  char rgbValue[6];
  sprintf(rgbValue, "%02X%02X%02X", redHTML, greenHTML, blueHTML);
  chestHTML += rgbValue;
  chestHTML += "';\n";
  chestHTML += "context.fill();\n";
  chestHTML += "context.stroke();\n";
  chestHTML += "context.closePath();\n";
  chestHTML += "context.restore();\n";

  chestHTML += "context.strokeStyle = '#000000';\n";
  chestHTML += "context.beginPath();\n";
  chestHTML += "context.moveTo(0, 32);\n";
  chestHTML += "context.lineTo(9, 32);\n";
  chestHTML += "context.lineWidth=5;\n";
  chestHTML += "context.stroke();\n";
  chestHTML += "context.closePath();\n";

  chestHTML += "context.strokeStyle = '#000000';\n";
  chestHTML += "context.beginPath();\n";
  chestHTML += "context.moveTo(55, 32);\n";
  chestHTML += "context.lineTo(64, 32);\n";
  chestHTML += "context.stroke();\n";
  chestHTML += "context.closePath();\n";
  chestHTML += "</script>\n";

  return chestHTML;
}

