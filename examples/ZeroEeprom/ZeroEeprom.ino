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
/* Example used in following API documentation:
    setUserData()
    getUserData()
*/

#include <mip.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RemoteDebug.h>

const char* ssid = "........";
const char* password = "........";
const char* hostname = "MiP-0x01";

MiP         mip;
RemoteDebug Debug;
bool        connectResult;

uint8_t eepromContents;

bool singleRun = true; // This example will run once.
const int wait = 5000; // Wait five seconds before sending program output.
long lastChangeTime = 0;

void setup() {
  defaultInit();
}

void loop() {
  ArduinoOTA.handle();

  long now = millis();

  if (now > lastChangeTime + wait) {
    if (singleRun) {
      for (uint8_t i = 0x00; i <= 0x0F; i++) { // Variable i is the EEPROM address offset where we will start writing zeroes.
        mip.setUserData(i, 0x00);
        delay(500);
        DEBUG_I("0x2%X: 0x%02X\n", i, mip.getUserData(i));
      }
      singleRun = false;
    }
    lastChangeTime = now;
  }

  Debug.handle();
}

void defaultInit() {
  // Bring up wifi first.  It will give MiP a chance to be ready.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.begin();

  // Start the debugging telnet server with hostname set.
  Debug.begin(hostname);

  // Allow a reset to the ESP8266 from the telnet client.
  Debug.setResetCmdEnabled(true);

  connectResult = mip.begin();
}

