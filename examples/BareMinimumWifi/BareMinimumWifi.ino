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
// This example sketch shows the bare minimum needed to connect MiP to wifi.
// This sketch may be used as a starting point for your sketch.
#include <mip_esp8266.h>

const char* ssid = "..............";          // Enter the SSID for your wifi network.
const char* password = "..............";      // Enter your wifi password.

const char* hostname = "MiP-0x01";            // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  Serial1.print("IP address: ");               // You could delete this chunk of code.  It's just here
  Serial1.println(WiFi.localIP());             // to show your IP address.
}

void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.

  // Put your amazing code here.



  /////////////////////////////////////////////////////////////////////////////////////////

}

