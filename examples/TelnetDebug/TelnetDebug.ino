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
    DEBUG()
    DEBUG_V()
    DEBUG_D()
    DEBUG_I()
    DEBUG_W()
    DEBUG_E()
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

long int lastTimeCheck;                       // Let's use a non-blocking delay.
const int period = 30000;                     // Give the user time to open a telnet terminal.

bool runOnce = true;

void setup() {
  defaultInit();                              // See at defaultInit() below. It handles all connections.

  Serial.println(F("TelnetDebug.ino - Explore the different telnet debug levels."));
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(F("Use serial debugging in setup()."));

  lastTimeCheck = millis();
}

void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.

  long now = millis();

  if (now > lastTimeCheck + period) {
    if (runOnce) {
      DEBUG("The telnet debug utility is very helpful.  It can selectively print messages of different levels.\n");
      DEBUG("Messages at the DEBUG level always print to telnet.\n");
      DEBUG("All debugging messages can use formatted output.\n");

      DEBUG_V("This is a verbose message.\n");
      DEBUG_D("This is a debug message.\n");
      DEBUG_I("This is an informational message.\n");
      DEBUG_W("This is a warning message.\n");
      DEBUG_E("This is an error message.\n");

      runOnce = false;
    }
  }

  Debug.handle();                             // Without this we can't debug MiP using telnet.
}

// Do not change anything below this line.  Beyond here lies wifi and MiP connections.

void defaultInit() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);                        // Bring up wifi first.  It will give MiP a chance to be ready.
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
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
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Debug.begin(hostname);                      // Start the debugging telnet server with hostname set.

  Debug.setResetCmdEnabled(true);             // Allow a reset to the ESP8266 from the telnet client.

  connectResult = mip.begin();                // Establish the connection between the D1 mini and MiP.
  if (!connectResult) {
    Serial.println(F("Failed connecting to MiP!"));
    return;
  }
}
