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
    enableClapEvents()
    disableClapEvents()
    areClapEventsEnabled()
    writeClapDelay(uint16_t delay)
    readClapDelay()
    availableClapEvents()
    readClapEvent()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("Clap.ino - Use clap related functions."));

  Serial1.println(F("Calling disableClapEvents()"));
  mip.disableClapEvents();
  bool isEnabled = mip.areClapEventsEnabled();
  Serial1.print(F("areClapEventsEnabled() returns "));
  if (isEnabled) {
    Serial1.println(F("true - fail"));
  } else {
    Serial1.println(F("false - pass"));
  }

  Serial1.println(F("Calling writeClapDelay(501)"));
  mip.writeClapDelay(501);
  uint16_t delay = mip.readClapDelay();
  Serial1.print(F("readClapDelay() returns "));
  Serial1.println(delay);

  Serial1.println(F("Calling enableClapEvents()"));
  mip.enableClapEvents();
  isEnabled = mip.areClapEventsEnabled();
  Serial1.print(F("areClapEventsEnabled() returns "));
  if (isEnabled) {
    Serial1.println(F("true - pass"));
  } else {
    Serial1.println(F("false - fail"));
  }

  Serial1.println();
  Serial1.println(F("Waiting for clap events!"));
}

void loop() {
  while (mip.availableClapEvents() > 0) {
    uint8_t clapCount = mip.readClapEvent();
    Serial1.print(F("Detected "));
      Serial1.print(clapCount);
      Serial1.println(F(" claps"));
  }
}

