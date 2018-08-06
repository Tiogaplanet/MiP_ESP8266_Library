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
    fallForward()
    fallBackward()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("FallDown.ino - Fall forward and backward.\n"));

  Serial1.println(F("Waiting for robot to be standing upright."));
  while (!mip.isUpright()) {
    // Waiting
  }
  delay(1000);

  Serial1.println(F("Falling forward."));
  mip.fallForward();

  delay(1000);
  Serial1.println(F("Waiting for robot to be standing upright again."));
  while (!mip.isUpright()) {
    // Waiting
  }
  delay(1000);

  Serial1.println(F("Falling backward."));
  mip.fallBackward();

  Serial1.println();
  Serial1.println(F("Sample done."));
}

void loop() {
}

