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
    writeHeadLEDs()
    readHeadLEDs()
    unverifiedWriteHeadLEDs()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("HeadLEDs.ino - Use head LED functions. Should set each head LED to different state."));
  mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_BLINK_SLOW, MIP_HEAD_LED_BLINK_FAST);

  MiPHeadLEDs headLEDs;
  mip.readHeadLEDs(headLEDs);
  Serial1.println(F("Head LEDs"));
  Serial1.print(F("    led1: "));
    printLEDString(headLEDs.led1);
  Serial1.print(F("    led2: "));
    printLEDString(headLEDs.led2);
  Serial1.print(F("    led3: "));
    printLEDString(headLEDs.led3);
  Serial1.print(F("    led4: "));
    printLEDString(headLEDs.led4);

  delay(4000);

  // Turn all the LEDs back on now.
  Serial1.println(F("Turning all eye LEDs back on now."));
  headLEDs.led1 = headLEDs.led2 = headLEDs.led3 = headLEDs.led4 = MIP_HEAD_LED_ON;
  mip.writeHeadLEDs(headLEDs);
  delay(1000);

  // Attempt to run through the same sequence of head LED changes using the 
  // unverifiedWriteHeadLEDs() functions which don't always get accepted by MiP.
  Serial1.println(F("Trying to set each head LED to a different state."));
  mip.unverifiedWriteHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_BLINK_SLOW, MIP_HEAD_LED_BLINK_FAST);
  delay(4000);

  Serial1.println(F("Trying to set all eye LEDs back on now."));
  headLEDs.led1 = headLEDs.led2 = headLEDs.led3 = headLEDs.led4 = MIP_HEAD_LED_ON;
  mip.unverifiedWriteHeadLEDs(headLEDs);

  Serial1.println();
  Serial1.println(F("Sample done."));
}

static void printLEDString(MiPHeadLED led) {
  switch (led) {
    case MIP_HEAD_LED_OFF:
      Serial1.println(F("Off"));
      break;
    case MIP_HEAD_LED_ON:
      Serial1.println(F("On"));
      break;
    case MIP_HEAD_LED_BLINK_SLOW:
      Serial1.println(F("Blink Slow"));
      break;
    case MIP_HEAD_LED_BLINK_FAST:
      Serial1.println(F("Blink Fast"));
      break;
    default:
      Serial1.println();
      break;
  }
}

void loop() {
}
