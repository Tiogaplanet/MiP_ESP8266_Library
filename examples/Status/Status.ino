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
    readBatteryVoltage();
    readPosition();
    isOnBack();
    isFaceDown();
    isUpright();
    isPickedUp();
    isHandStanding();
    isFaceDownOnTray();
    isOnBackWithKickstand();
*/

#include <mip.h>
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

void setup() {
  defaultInit();
}

void loop() {
  ArduinoOTA.handle();

  static float       lastBatteryLevel = 0.0f;
  static MiPPosition lastPosition = (MiPPosition) - 1;

  float              currentBatteryLevel = mip.readBatteryVoltage();
  MiPPosition        currentPosition = mip.readPosition();

  if (currentBatteryLevel != lastBatteryLevel) {
    DEBUG_I("Battery: %fV\n", currentBatteryLevel);
    lastBatteryLevel = currentBatteryLevel;
  }

  if (currentPosition != lastPosition) {
    if (mip.isOnBack()) {
      DEBUG_I("Position: On Back\n");
    }
    if (mip.isFaceDown()) {
      DEBUG_I("Position: Face Down\n");
    }
    if (mip.isUpright()) {
      DEBUG_I("Position: Upright\n");
    }
    if (mip.isPickedUp()) {
      DEBUG_I("Position: Picked Up\n");
    }
    if (mip.isHandStanding()) {
      DEBUG_I("Position: Hand Stand\n");
    }
    if (mip.isFaceDownOnTray()) {
      DEBUG_I("Position: Face Down on Tray\n");
    }
    if (mip.isOnBackWithKickstand()) {
      DEBUG_I("Position: On Back With Kickstand\n");
    }

    lastPosition = currentPosition;
  }
  
  Debug.handle();
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

