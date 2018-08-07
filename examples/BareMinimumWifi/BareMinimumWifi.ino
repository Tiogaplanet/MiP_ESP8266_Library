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
  defaultInit();                              // See at defaultInit() below. It handles all connections.

  Serial1.print("IP address: ");               // You could delete this chunk of code.  It's just here
  Serial1.println(WiFi.localIP());             // to show your IP address.
}

void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.

  // Put your amazing code here, betwen the call to ArduinoOTA.handle() and Debug.handle().



  /////////////////////////////////////////////////////////////////////////////////////////

  Debug.handle();                             // Without this we can't debug MiP using telnet.
}

// Do not change anything below this line.  Beyond here lies wifi and MiP connections.

void defaultInit() {
  connectResult = mip.begin();                // Establish the connection between the D1 mini and MiP.
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }
  Serial1.println(F("Booting..."));

  WiFi.mode(WIFI_STA);                        // Next, bring up wifi.
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial1.println(F("Connection Failed! Rebooting..."));
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(hostname);           // Pass the hostname to the OTA support.

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial1.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial1.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial1.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial1.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial1.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial1.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial1.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial1.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial1.println(F("End Failed"));
  });

  ArduinoOTA.begin();

  Debug.begin(hostname);                      // Start the debugging telnet server with hostname set.

  Debug.setResetCmdEnabled(true);             // Allow a reset to the ESP8266 from the telnet client.
}
