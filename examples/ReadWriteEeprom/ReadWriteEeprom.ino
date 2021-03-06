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
    setUserData()
    getUserData()
*/
#include <mip_esp8266.h>

MiP           mip;

// Use an offset between 0x00 and 0x0F.
const uint8_t eepromAddressOffset = 0x00;

// Try different hex values here to see them stored and
// recovered from EEPROM.
uint8_t       secretPassword = 0x0D;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("ReadWriteEeprom.ino - Writes data to EEPROM and reads it back."));

  Serial1.print(F("Original password: "));
  Serial1.println(secretPassword, HEX);

  // Power-off the MiP, comment out this line, recompile and load to the ProMini-Pack to see EEPROM
  // data preserved across power cycles.
  mip.setUserData(eepromAddressOffset, secretPassword);

  // "Scramble" the secret password.
  secretPassword = 0xFF;
  Serial1.print(F("Scrambled password: "));
  Serial1.println(secretPassword, HEX);

  Serial1.print(F("Recovered password: "));
  Serial1.print(mip.getUserData(eepromAddressOffset), HEX);
}

void loop() {
}

