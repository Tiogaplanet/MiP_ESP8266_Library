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
// This example sketch turns MiP into a weather person and writes the
// weather data to telnet.
#include <mip_esp8266.h>
#include <mip_debug.h>
#include <JsonListener.h>
#include <time.h>
#include "OpenWeatherMapCurrent.h"

// Initiate the client for the OpenWeatherMap API.
OpenWeatherMapCurrent client;

// Provide your OpenWeatherMap API key.  See
// https://docs.thingpulse.com/how-tos/openweathermap-key/
// for more information.
String OPEN_WEATHER_MAP_APP_ID = "your_openweathermap_api_key";

// Provide the OpenWeatherMap ID for your city.  For example, the value for Naples, Italy
// is 3172394 and Charleston, South Carolina is 4574324.
String OPEN_WEATHER_MAP_LOCATION_ID = "4574324";

String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = false;

// Enter the SSID for your wifi network.
const char* ssid = "..............";

// Enter your wifi password.
const char* password = "..............";

// Set any hostname you desire.
const char* hostname = "MiP-0x01";

MiP         mip;
bool        connectResult;

MiPDebug debug;

// For the chest LED.
uint8_t red, green, blue = 0;
bool chestValuesWritten = false;

// Store the last time OpenWeatherMap was queried.
unsigned long previousMillis = 0;

// Update every 15 minutes (900000 milliseconds).
const long interval = 900000;

// A place to store the data retrieved from OpenWeatherMap.
OpenWeatherMapCurrentData data;

// Don't update the eyes if they're already solid.
bool lastUpdatedToSolid = false;

// Write the weather data to telnet every 15 minutes.
bool debugDataToTelnet = false;

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  debug.begin(hostname);

  // We'll need a random number generator to animate the eyes in conditions of rain.
  randomSeed(analogRead(A0));

  // Get the weather for the first time.
  updateWeather();
}

void loop() {
  ArduinoOTA.handle();

  // Now, get the weather every 15 minutes.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Call the function to read from OpenWeatherMap.
    updateWeather();
    chestValuesWritten = false;
    debugDataToTelnet = false;
    previousMillis = currentMillis;
  }

  // Animate the eyes to indicate rain.
  if (data.description.indexOf("rain") > 0) {
    mip.writeHeadLEDs((MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2));
    lastUpdatedToSolid = false;
  } else if (!lastUpdatedToSolid) {
    mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON);
    lastUpdatedToSolid = true;
  }

  // Update the chest LED if it hasn't been done in the last 15 minutes.
  if (!chestValuesWritten) {
    chestValuesWritten = updateChestLED();
  }

  // Write to telnet if it hasn't been done in the last 15 minutes.
  if (!debugDataToTelnet) {
    debugDataToTelnet = debugToTelnet();
  }

  // Don't update the eyes too fast or you'll get MiP timeout errors.
  delay(800);

  // Write to telnet.
  debug.handle();
}

// Read the weather from OpenWeatherMap.
void updateWeather() {
  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  client.setMetric(IS_METRIC);
  client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
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

// Setup the debug data to send to telnet.
bool debugToTelnet() {
  mDebugI("---------------------------------------------------\n\r");
  mDebugI("City: %s\n\r", data.cityName.c_str());
  mDebugI("Country: %s\n\r", data.country.c_str());
  mDebugI("Longitude: %f\n\r", data.lon);
  mDebugI("Latitude: %f\n\r", data.lat);
  time_t time = data.observationTime;
  mDebugI("Observation time: %s\r", ctime(&time));
  mDebugI("Weather ID: %d\n\r", data.weatherId);
  mDebugI("Main: %s\n\r", data.main.c_str());
  mDebugI("Description: %s\n\r", data.description.c_str());
  mDebugI("Icon: %s\n\r", data.icon.c_str());
  mDebugI("IconMeteoCon: %s\n\r", data.iconMeteoCon.c_str());
  mDebugI("Temperature: %f\n\r", data.temp);
  mDebugI("Pressure: %d\n\r", data.pressure);
  mDebugI("Humidity: %d\n\r", data.humidity);
  mDebugI("Temperature minimum: %f\n\r", data.tempMin);
  mDebugI("Temperature maximum: %f\n\r", data.tempMax);
  mDebugI("Wind speed: %f\n\r", data.windSpeed);
  mDebugI("Wind degrees: %f\n\r", data.windDeg);
  mDebugI("Clouds: %d\n\r", data.clouds);
  time = data.sunrise;
  mDebugI("Sunrise: %s\r", ctime(&time));
  time = data.sunset;
  mDebugI("Sunset: %s\r", ctime(&time));
  mDebugI("---------------------------------------------------\n\r");
  mDebugI("Chest LED values: \n\r");
  mDebugI("Red: %d\n\r", red);
  mDebugI("Green: %d\n\r", green);
  mDebugI("Blue: %d\n\r", blue);

  return true;
}

