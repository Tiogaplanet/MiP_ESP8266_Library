/* Copyright (C) 2018  Adam Green (https://github.com/adamgreen)

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
    sleep()
*/

#include <mip_esp8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RemoteDebug.h>

const char* ssid = "..............";          // Enter the SSID for your wifi network.
const char* password = "..............";      // Enter your wifi password.
const char* hostname = "MiP-0x01";            // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
RemoteDebug Debug;                            // and a single Debug object.
bool        connectResult;                    // Test whether a connection to MiP was established.

bool singleRun = true; // This example will run once.
const int wait = 10000; // Wait five seconds before sending program output.
long lastChangeTime = 0;
long now;

bool ended = false;
bool reconnected = false;

void setup() {
  defaultInit();

  lastChangeTime = millis();
}

void loop() {
  ArduinoOTA.handle();

  now = millis();

  if (connectResult && now > lastChangeTime + wait) {
    DEBUG_I("Disconnecting from MiP. Chest LED should turn blue.\n");
    mip.end();
    ended = true;
    connectResult = false;
    lastChangeTime = now;
  }

  now = millis();

  if (ended && now > lastChangeTime + wait) {
    DEBUG_I("Attempting to reconnect to MiP. Chest LED should turn green again.\n");
    mip.begin();
    ended = false;
    reconnected = true;
    lastChangeTime = now;
  }

  now = millis();

  if (reconnected && now > lastChangeTime + wait) {
    DEBUG_I("Putting MiP to sleep. Will require power cycle before it will accept UART connections again.\n");
    mip.sleep();
    reconnected = false;
  }

  defaultMessaging();
}

void defaultInit() {
  WiFi.mode(WIFI_STA);                        // Bring up wifi first.  It will give MiP a chance to be ready.
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP.restart();
  }

  ArduinoOTA.setHostname(hostname);           // Pass the hostname to the OTA support.

  ArduinoOTA.begin();

  Debug.begin(hostname);                      // Start the debugging telnet server with hostname set.

  Debug.setResetCmdEnabled(true);             // Allow a reset to the ESP8266 from the telnet client.

  connectResult = mip.begin();                // Establish the connection between the D1 mini and MiP.
}

void defaultMessaging() {
  DEBUG_D(mip.dumpDebug());                   // Debug-level messages dumped by the API.
  DEBUG_I(mip.dumpInfo());                    // Informational messages dumped by the API.
  DEBUG_E(mip.dumpErrors());                  // Error messages dumped by the API.
  Debug.handle();                             // Handle sending messages via telnet.
}

