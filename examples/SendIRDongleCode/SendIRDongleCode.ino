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
    sendIRDongleCode()
*/

#include <mip_esp8266.h>
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

// Try different values for VALID_DATA_BYTES (2, 3 or 4)
#define VALID_DATA_BYTES 4

// Try different values for transmission power (0x01 - 0x78)
#define MIP_IR_TX_POWER  0x78

#define MAX_DATA_BYTES 4

bool connectResult;

// Try different codes for dongleCode[]. For valid codes visit
// https://github.com/Tiogaplanet/MiP_ESP8266_Library/wiki/Infrared#sendirdonglecode
uint8_t dongleCode[4] = { 0xBB, 0x0AB, 0xFF, 0xFF };

void setup() {
  defaultInit();
}

void loop() {
  ArduinoOTA.handle();

  DEBUG_I("Sending ");
  for (int i = MAX_DATA_BYTES - VALID_DATA_BYTES; i < MAX_DATA_BYTES; i++) {
    DEBUG_I("0x%02X ", dongleCode[i]);
  }
  DEBUG_I("\n");

  mip.sendIRDongleCode(dongleCode, VALID_DATA_BYTES, MIP_IR_TX_POWER);

  delay(1000);

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
  DEBUG_D(mip.dumpDebug());
  DEBUG_I(mip.dumpInfo());
  DEBUG_E(mip.dumpErrors());
  Debug.handle();
}

