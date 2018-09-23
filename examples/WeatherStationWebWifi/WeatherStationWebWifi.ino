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
#include "OpenWeatherMapCurrent.h"

// Include the WebServer library.
#include <ESP8266WebServer.h>

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
char* ssid = "..............";

// Enter your wifi password.
char* password = "..............";

// Set any hostname you desire.
char* hostname = "MiP-0x02";

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

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

// Set web server port number to 80.
ESP8266WebServer server(80);

// Variable in which to store the HTTP request.
String header;

// function prototypes for HTTP handlers.
void handleRoot();
void handleNotFound();

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  // We'll need a random number generator to animate the eyes in conditions of rain.
  randomSeed(analogRead(A0));

  // Call the 'handleRoot' function when a client requests URI "/".
  server.on("/", handleRoot);

  // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound."
  server.onNotFound(handleNotFound);

  // Start the web server.
  server.begin();

  updateWeather();
}

void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    updateWeather();
    chestValuesWritten = false;
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

  // Don't update the eyes too fast or you'll get MiP timeout errors.
  delay(800);

  // Update the chest LED if it hasn't been done in the last 15 minutes.
  if (!chestValuesWritten) {
    chestValuesWritten = updateChestLED();
  }

  // Listen for HTTP requests from clients.
  server.handleClient();
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

void handleRoot() {
  String htmlOutput = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n";
  htmlOutput += "<title>MiP, the Weather Person</title>\n";
  htmlOutput += "<style>\n";
  htmlOutput += "body {background-color: powderblue;}\n";
  htmlOutput += "h1   {color: blue; font-family: Arial, Helvetica, sans-serif; font-size: 120%;}\n";
  htmlOutput += "p    {color: black; font-family: Arial, Helvetica, sans-serif;}\n";
  htmlOutput += "</style>\n";
  htmlOutput += "</head>\n<body>\n";
  htmlOutput += "<div class=\"Weather\">\n";
  htmlOutput += "<h1>Weather data courtesy of OpenWeatherMap</h1>\n";
  htmlOutput += "<p>City: " + data.cityName + "<br>\n";
  htmlOutput += "Country: " + data.country + "<br>\n";

  htmlOutput += "Longitude: " + String(data.lon) + "<br>\n";
  htmlOutput += "Latitude: " + String(data.lat) + "<br>\n";
  time_t time = data.observationTime;
  htmlOutput += "Observation time: " + String(ctime(&time)) + "<br>\n";
  htmlOutput += "Weather ID: " + String(data.weatherId) + "<br>\n";
  htmlOutput += "Main: " + data.main + "<br>\n";
  htmlOutput += "Description: " + data.description + "<br>\n";
  htmlOutput += "Icon: " + data.icon + "<br>\n";
  htmlOutput += "IconMeteoCon: " + data.iconMeteoCon + "<br>\n";
  htmlOutput += "Temperature: " + String(data.temp) + "<br>\n";
  htmlOutput += "Pressure: " + String(data.pressure) + "<br>\n";
  htmlOutput += "Humidity: " + String(data.humidity) + "<br>\n";
  htmlOutput += "Temperature minimum: " + String(data.tempMin) + "<br>\n";
  htmlOutput += "Temperature maximum: " + String(data.tempMax) + "<br>\n";
  htmlOutput += "Wind speed: " + String(data.windSpeed) + "<br>\n";
  htmlOutput += "Wind degrees: " + String(data.windDeg) + "<br>\n";
  htmlOutput += "Clouds: " + String(data.clouds) + "<br>\n";
  time = data.sunrise;
  htmlOutput += "Sunrise: " + String(ctime(&time)) + "<br>\n";
  time = data.sunset;
  htmlOutput += "Sunset: " + String(ctime(&time)) + "<br>\n";
  htmlOutput += "</p>\n</div>\n";

  // Show the RGB values for the chest LED.
  htmlOutput += "<div class=\"Chest LED\">\n";
  htmlOutput += "<h1>Chest LED</h1>\n";
  htmlOutput += "<p>Red: " + String(red) + "<br>\n";
  htmlOutput += "Green: " + String(green) + "<br>\n";
  htmlOutput += "Blue: " + String(blue) + "<br>\n";
  htmlOutput += "</p>\n</div>\n</body>\n</html>\n";

  // Send HTTP status 200 (Ok) and the page to the client.
  server.send(200, "text/html", htmlOutput);
}

void handleNotFound() {
  // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request.
  server.send(404, "text/plain", "404: Not found");
}

