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
#include <mip_esp8266.h>
#include <mip_debug.h>

char* ssid = "..............";                // Enter the SSID for your wifi network.
char* password = "..............";            // Enter your wifi password.

char* hostname = "MiP-0x01";                  // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

MiPDebug Debug;                               // For debugging over telnet.

long int lastTimeCheck;                       // Let's use a non-blocking delay.
const int period = 30000;                     // Give the user time to open a telnet terminal.

bool runOnce = true;

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  Debug.begin(hostname, Debug.ANY);

  Debug.setResetCmdEnabled(true);             // Enable the reset command

  Serial1.println(F("TelnetDebug.ino - Explore the different telnet debug levels."));
  Serial1.println();
  Serial1.print("IP address: ");
  Serial1.println(WiFi.localIP());
  Serial1.println(F("Use serial debugging in setup()."));

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

    DEBUG_V("* This is a message of debug level VERBOSE\n");
    DEBUG_D("* This is a message of debug level DEBUG\n");
    DEBUG_I("* This is a message of debug level INFO\n");
    DEBUG_W("* This is a message of debug level WARNING\n");
    DEBUG_E("* This is a message of debug level ERROR\n");

    lastTimeCheck = now;
  }

  Debug.handle();                            // Without this we can't debug MiP using telnet.
}

