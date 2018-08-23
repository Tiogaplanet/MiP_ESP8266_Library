/**The MIT License (MIT)

  Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
// This example sketch shows how to turn MiP into a weather person.
#include <mip_esp8266.h>
#include <JsonListener.h>
#include <time.h>
#include "OpenWeatherMapCurrent.h"

// initiate the client
OpenWeatherMapCurrent client;

// See https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "your_openweathermap_app_id";
/*
  Go to https://openweathermap.org/find?q= and search for a location. Go through the
  result set and select the entry closest to the actual location you want to display
  data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
  at the end is what you assign to the constant below.  The value for Naples, Italy
  is 3172394.
*/
String OPEN_WEATHER_MAP_LOCATION_ID = "3172394";
/*
  Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
  English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
  Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
  Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
  Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
  Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
  Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
*/
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = false;

char* ssid = "..............";                // Enter the SSID for your wifi network.
char* password = "..............";            // Enter your wifi password.

char* hostname = "MiP-0x01";                  // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.


void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  Serial1.println();
  Serial1.println("\n\nNext Loop-Step: " + String(millis()) + ":");

  OpenWeatherMapCurrentData data;
  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  client.setMetric(IS_METRIC);
  client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);

  Serial1.println(F("------------------------------------"));
  Serial1.printf("City: %s\n\r", data.cityName.c_str());
  Serial1.printf("Country: %s\n\r", data.country.c_str());
  Serial1.printf("Longitude: %f\n\r", data.lon);
  Serial1.printf("Latitude: %f\n\r", data.lat);
  time_t time = data.observationTime;
  Serial1.printf("Observation time: %s\r", ctime(&time));
  Serial1.printf("Weather ID: %d\n\r", data.weatherId);
  Serial1.printf("Main: %s\n\r", data.main.c_str());
  Serial1.printf("Description: %s\n\r", data.description.c_str());
  Serial1.printf("Icon: %s\n\r", data.icon.c_str());
  Serial1.printf("IconMeteoCon: %s\n\r", data.iconMeteoCon.c_str());
  Serial1.printf("Temperature: %f\n\r", data.temp);
  Serial1.printf("Pressure: %d\n\r", data.pressure);
  Serial1.printf("Humidity: %d\n\r", data.humidity);
  Serial1.printf("Temperature minimum: %f\n\r", data.tempMin);
  Serial1.printf("Temperature maximum: %f\n\r", data.tempMax);
  Serial1.printf("Wind speed: %f\n\r", data.windSpeed);
  Serial1.printf("Wind degrees: %f\n\r", data.windDeg);
  Serial1.printf("Clouds: %d\n\r", data.clouds);
  time = data.sunrise;
  Serial1.printf("Sunrise: %s\r", ctime(&time));
  time = data.sunset;
  Serial1.printf("Sunset: %s\r", ctime(&time));
  Serial1.println();
  Serial1.println(F("---------------------------------------------------"));

  // This is where the magic happens.  Set MiP's chest to indicate the current temperature.
  // Using algorithm, but not the values, from:
  // https://sjackm.wordpress.com/2012/03/26/visualizing-temperature-as-color-using-an-rgb-led-a-lm35-sensor-and-arduino/

  uint8_t red, green, blue;

  if (data.temp < 32) {
    blue = 255;
  }
  else if (data.temp > 32 && data.temp <= 72) {
    blue = map(data.temp, 32, 72, 255, 0);
  }
  else if (data.temp > 72) {
    blue = 0;
  }

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
    green = map(data.temp, 80, 125, 255, 0);
  }

  if (data.temp < 72) {
    red = 0;
  }
  else if (data.temp >= 72 && data.temp <= 80) {
    red = map(data.temp, 72, 80, 1, 255);
  }
  else if (data.temp > 80) {
    red = 255;
  }

  mip.writeChestLED(red, green, blue);

  Serial1.print(F("R: "));Serial1.println(red);
  Serial1.print(F("G: "));Serial1.println(green);
  Serial1.print(F("B: "));Serial1.println(blue);
}

void loop() {
  // The chest LED could be updated for example, every fifteen minutes.
}
