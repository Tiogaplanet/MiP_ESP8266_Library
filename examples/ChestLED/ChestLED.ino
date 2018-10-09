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
    writeChestLED()
    readChestLED()
    unverifiedChestLED()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("ChestLED.ino - Set Chest LED to different colors.\n"));

  Serial1.println(F("Set chest LED to magenta."));
  uint8_t red = 0xff;
  uint8_t green = 0x01;
  uint8_t blue = 0xfe;
  mip.writeChestLED(red, green, blue);
  printCurrentChestLEDSetting();
  delay(1000);

  Serial1.println(F("Set chest LED to blink red."));
  red = 0xff;
  green = 0x01;
  blue = 0x05;
  const uint16_t onTime = 990;
  const uint16_t offTime = 989;
  mip.writeChestLED(red, green, blue, onTime, offTime);
  printCurrentChestLEDSetting();
  delay(4000);

  Serial1.println(F("Set chest LED back to green."));
  MiPChestLED chestLED;
  chestLED.red = 0x00;
  chestLED.green = 0xff;
  chestLED.blue = 0x00;
  chestLED.onTime = 0;
  chestLED.offTime = 0;
  mip.writeChestLED(chestLED);
  printCurrentChestLEDSetting();
  delay(1000);

  // Attempt to run through the same sequence of chest LED changes using the 
  // unverifiedWriteChestLED() functions which don't always get accepted by MiP.
  Serial1.println(F("Trying to set chest LED to magenta."));
  red = 0xff;
  green = 0x01;
  blue = 0xfe;
  mip.unverifiedWriteChestLED(red, green, blue);
  delay(1000);

  Serial1.println(F("Trying to set chest LED to blink red."));
  red = 0xff;
  green = 0x01;
  blue = 0x05;
  mip.unverifiedWriteChestLED(red, green, blue, onTime, offTime);
  delay(4000);

  Serial1.println(F("Trying to set chest LED back to green."));
  chestLED.red = 0x00;
  chestLED.green = 0xff;
  chestLED.blue = 0x00;
  chestLED.onTime = 0;
  chestLED.offTime = 0;
  mip.unverifiedWriteChestLED(chestLED);
  delay(1000);
  
  Serial1.println();
  Serial1.println(F("Sample done."));
}

void loop() {
}

static void printCurrentChestLEDSetting() {
  MiPChestLED chestLED;
  mip.readChestLED(chestLED);

  Serial1.println(F("Current Chest LED Setting"));
  Serial1.print(F("    red: "));
    Serial1.println(chestLED.red);
  Serial1.print(F("    green: "));
    Serial1.println(chestLED.green);
  Serial1.print(F("    blue: "));
    Serial1.println(chestLED.blue);
  Serial1.print(F("    on time: "));
    Serial1.print(chestLED.onTime);
    Serial1.println(F(" milliseconds"));
  Serial1.print(F("    off time: "));
    Serial1.print(chestLED.offTime);
    Serial1.println(F(" milliseconds"));
  Serial1.println();
}
