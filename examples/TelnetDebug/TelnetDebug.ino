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
f#include <ArduinoOTA.h>
#include <RemoteDebug.h>

const char* ssid = "..............";          // Enter the SSID for your wifi network.
const char* password = "..............";      // Enter your wifi password.
const char* hostname = "MiP-0x01";            // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
RemoteDebug Debug;                            // and a single Debug object.
bool        connectResult;                    // Test whether a connection to MiP was established.

long int waitTime;
int period = 30000;                           // Give the user time to open a telnet terminal.

bool runOnce = true;

void setup() {
  defaultInit();                              // Look at defaultInit() below.  It does everything
                                              // needed to connect MiP to wifi.
  Serial.println(F("TelnetDebug.ino - Explore the different telnet debug levels.\n"));
}                                             

void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.
  
  
  if(runOnce) {
    DEBUG("The telnet debug utility is very helpful.  It can selectively print messages of different levels.");
    DEBUG("Messages at the DEBUG level always print to telnet.");
    DEBUG("All debugging messages can use formatted output.");

    DEBUG_V("This is a verbose message.");
    DEBUG_D("This is a debug message.");
    DEBUG_I("This is a informational message.");
    DEBUG_W("This is a warning message.");
    DEBUG_E("This is a error message.");
    
    runOnce = false;
  }
  
  Debug.handle();                             // Without this we can't debug MiP using telnet.
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
