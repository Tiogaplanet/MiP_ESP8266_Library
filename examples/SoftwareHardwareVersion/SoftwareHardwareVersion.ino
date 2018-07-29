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
    readSoftwareVersion()
    readHardwareInfo()
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

bool singleRun = true;                        // This example will run once.
const int wait = 5000;                        // Wait five seconds before sending program output.
long lastChangeTime = 0;

void setup()
{
  defaultInit();
}

void loop()
{
  ArduinoOTA.handle();

  long now = millis();

  if (now > lastChangeTime + wait) {
    if (singleRun) {
      MiPSoftwareVersion softwareVersion;
      mip.readSoftwareVersion(softwareVersion);
      DEBUG_I("software version: %d-%d-%d.%d\n", softwareVersion.year, softwareVersion.month, softwareVersion.day, softwareVersion.uniqueVersion);

      MiPHardwareInfo hardwareInfo;
      mip.readHardwareInfo(hardwareInfo);
      DEBUG_I("hardware info\n");
      DEBUG_I("  voice chip version: %d\n", hardwareInfo.voiceChip);
      DEBUG_I("  hardware version: %d\n", hardwareInfo.hardware);

      singleRun = false;
    }
    lastChangeTime = now;
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

