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
// Turn MiP into a clock! This example reads network time protocol (NTP) and
// writes the time to MiP's eyes, one digit at a time.
#include <mip_esp8266.h>
#include <time.h>                             // We'll read the time and parse it.

const char* ssid = "..............";          // Enter the SSID for your wifi network.
const char* password = "..............";      // Enter your wifi password.

const char* hostname = "MiP-0x01";            // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

void setup() {
  configTime(-4 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial1.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial1.print(".");
    delay(1000);
  }
}

void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.

  time_t now = time(nullptr);                 // Read the time from NTP.
  struct tm * timeinfo;
  Serial1.println(ctime(&now));
  timeinfo = localtime(&now);

  uint8_t hour_tens = timeinfo->tm_hour / 10; // Parse the time into individual numbers.
  uint8_t hour_ones = timeinfo->tm_hour % 10;
  uint8_t minute_tens = timeinfo->tm_min / 10;
  uint8_t minute_ones = timeinfo->tm_min % 10;

  Serial1.print(F("Hour tens: ")); Serial1.println(hour_tens);
  Serial1.print(F("Hour ones: ")); Serial1.println(hour_ones);
  Serial1.print(F("Minute tens: ")); Serial1.println(minute_tens);
  Serial1.print(F("Minute ones: ")); Serial1.println(minute_ones);

  switch (hour_tens) {                        // In the next four switches, write the time to MiP's eyes.
    case 0:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 1:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 2:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
  }

  delay(2000);
  mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
  delay(500);

  switch (hour_ones) {
    case 0:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 1:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 2:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 3:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 4:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 5:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 6:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 7:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 8:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON);
      break;
    case 9:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON);
      break;
  }

  delay(2000);
  mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
  delay(500);

  switch (minute_tens) {
    case 0:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 1:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 2:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 3:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 4:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 5:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
  }

  delay(2000);
  mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
  delay(500);

  switch (minute_ones) {
    case 0:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 1:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 2:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 3:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      break;
    case 4:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 5:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 6:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 7:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF);
      break;
    case 8:
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON);
      break;
    case 9:
      mip.writeHeadLEDs(MIP_HEAD_LED_ON, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON);
      break;
  }

  delay(2000);
  mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
  delay(3000);
}

