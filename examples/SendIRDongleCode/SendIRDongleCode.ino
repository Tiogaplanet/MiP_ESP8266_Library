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
    sendIRDongleCode()
*/

#include <mip.h>

// Try different values for VALID_DATA_BYTES (2, 3 or 4)
#define VALID_DATA_BYTES 4

// Try different values for transmission power (0x01 - 0x78)
#define MIP_IR_TX_POWER  0x78

#define MAX_DATA_BYTES 4

MiP  mip;
bool connectResult;

// Try different codes for dongleCode[]. For valid codes, refer to readme.md included with this library or visit
// https://github.com/adamgreen/MiP_ProMini-Pack/blob/master/Arduino/MiP_ProMini_Pack_Library/README.md
const uint8_t dongleCode[4] = { 0xBB, 0x0AB, 0xFF, 0xFF };

void setup() {
  connectResult = mip.begin();
  if (!connectResult)
  {
    Serial.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial.println(F("SendIRDongleCode.ino - Send code to another MiP using IR."));
}

void loop() {
  Serial.print(F("Sending "));
  for (int i = MAX_DATA_BYTES - VALID_DATA_BYTES; i < MAX_DATA_BYTES; i++) {
    Serial.print(dongleCode[i], HEX); Serial.print(F(" "));
  }
  Serial.println();

  mip.sendIRDongleCode(dongleCode, VALID_DATA_BYTES, MIP_IR_TX_POWER);

  delay(1000);
}
