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
String OPEN_WEATHER_MAP_LOCATION_ID = "3172394";

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
bool extinguished = false;

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
  mip.writeClapDelay(501);
  mip.enableClapEvents();
}

void loop() {
  ArduinoOTA.handle();

  while (mip.availableClapEvents() > 0) {
    uint8_t clapCount = mip.readClapEvent();
    if (clapCount == 1 && extinguished == false) {
      Serial1.println(F("Switching off."));
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      mip.writeChestLED(0, 0, 0);
      extinguished = true;
    } else if (extinguished == true) {
      Serial1.println(F("Switching on."));
      extinguished = false;
      // Animate the eyes to indicate rain.
      if (data.description.indexOf("rain") > 0) {
        mip.writeHeadLEDs((MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2), (MiPHeadLED)random(0, 2));
        lastUpdatedToSolid = false;
      } else {
        mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON);
        lastUpdatedToSolid = true;
      }
      // Turn on the chest LED.
      chestValuesWritten = updateChestLED();
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    updateWeather();
    chestValuesWritten = false;
    previousMillis = currentMillis;
  }

  if (!extinguished) {
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
  // Send HTTP status 200 (Ok) and the page to the client.
  //server.send(200, "text/html", htmlOutput);
  server.send(200, "text/html", completePage());
}

void handleNotFound() {
  // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request.
  server.send(404, "text/plain", "404: Not found");
}

// Beyond here lies the HTML.
//////////////////////////////////////////////////////////////////////////

String completePage() {
  String htmlOutput = "<!DOCTYPE html>\n<html>\n";
  htmlOutput += htmlHead();
  htmlOutput += htmlBody() ;
  htmlOutput += "</html>\n";

  return htmlOutput;
}

String htmlHead() {
  String head = "<head>\n";

  // TODO: Favicon should be the weather condition icon.
  head += "<link rel=\"icon\" href=\"http://openweathermap.org/img/w/" + data.icon + ".png\">\n";

  // Refresh every 15 minutes.
  head += "<meta http-equiv=\"refresh\" content=\"900\">\n";
  head += " <meta charset=\"UTF-8\">\n";

  head += "<style>\n";
  head += "  html, body {height: 100%;}\n";
  head += "  html {display: table; margin: auto;}\n";
  head += "  body {background-color: ";
  if (data.description == "clear sky") {
    head += "#2572ed";
  } else if (data.description == "few clouds") {
    head += "#537bba";
  } else if (data.description == "scattered clouds") {
    head += "#5371a0";
  } else if (data.description == "broken clouds") {
    head += "#52698e";
  } else if (data.description == "shower rain") {
    head += "#4c5b72";
  } else if (data.description == "rain") {
    head += "#444d5b";
  } else if (data.description == "thunderstorm") {
    head += "#3d434c";
  } else if (data.description == "snow") {
    head += "#d5dce8";
  } else if (data.description == "mist") {
    head += "#bcbfc4";
  }
  head += "; display: table-cell;}\n"; // vertical-align: middle;
  head += "  h1 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 200%; text-align: center; line-height: 5px;}\n";
  head += "  h2 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 300%; text-align: center; line-height: 5px;}\n";
  head += "  h3 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 110%; text-align: center; line-height: 5px;}\n";
  head += "  h4 {color: white; font-family: Arial, Helvetica, sans-serif; font-size: 100%; text-align: center; line-height: 5px;}\n";
  head += "  hr {border-top: 1px solid white;}\n";
  head += "  p {color: white; font-family: Arial, Helvetica, sans-serif;}\n";
  head += "  .weather {border:1px solid white; border-radius: 20px; background-color: #2b76ef; padding: 10px;}\n";
  head += "  canvas {padding-left: 0; padding-right: 0; margin-left: auto; margin-right: auto; display: block;}\n";
  head += "  footer {color: #d26c22; text-align: center;}\n";
  head += " </style>\n";

  head += "<title>" + data.cityName + " Weather Conditions</title>\n";

  head += " </head>\n";

  return head;
}

String htmlBody() {
  String body = "<body>\n";
  body += "<p/>\n";
  body += "<div class=\"weather\">\n";
  body += htmlHeader();
  body += htmlWeatherData();
  body += htmlFooter();
  body += "</div>\n";
  body += "<p/>\n";
  body += "</body>\n";

  return body;
}

// The header shows just the city and the temperature.
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

String htmlWeatherData() {
  String htmlOutput = "<hr />";
  htmlOutput += "<p>\n";
  time_t time = data.observationTime;
  htmlOutput += "Observation time: " + String(ctime(&time)) + "<br>\n";
  htmlOutput += "Weather ID: " + String(data.weatherId) + "<br>\n";
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
  if (extinguished) {
    htmlOutput += " is muted";
  }
  htmlOutput += "</h3>\n";
  if (!extinguished) {
    htmlOutput += chestHTML(red, green, blue);
    /*
    htmlOutput += "<p>\nRed: " + String(red) + "<br>\n";
    htmlOutput += "Green: " + String(green) + "<br>\n";
    htmlOutput += "Blue: " + String(blue) + "<br>\n";
    */
  }
  htmlOutput += "</p>\n";
  htmlOutput += "<hr />";

  return htmlOutput;
}

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

