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
    readIRDongleCode()
*/

#include <mip.h>

MiP       mip;
uint8_t   dongleCode[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
MiPIRCode receiveCode;
bool      connectResult;

void setup() {
  connectResult = mip.begin();
  if (!connectResult)
  {
    Serial.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial.println(F("ReadIRDongleCode.ino - Receive code from another MiP using IR."));
}

void loop() {
  mip.readIRDongleCode(receiveCode);

  if (receiveCode.dataNumbers) {
    Serial.print(F("Receiving ")); Serial.print(receiveCode.dataNumbers); Serial.println(F(" data numbers."));
    for (int i = 0; i < receiveCode.dataNumbers ; i++)
    {
      Serial.print(receiveCode.code[i], HEX); Serial.print(F(" "));
    }
    Serial.println();
  }

  receiveCode.clear();

  delay(500);
}
