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
    availableIRCodeEvents()
*/
#include <mip_esp8266.h>

MiP       mip;
bool      connectResult;

uint32_t referenceCode = 0xFFFFFFFF;

void setup() {
  connectResult = mip.begin();
  if (!connectResult)
  {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("ReadIRDongleCode.ino - Receive code from another MiP using IR."));
}

void loop() {
  uint32_t receiveCode;

  if (mip.availableIRCodeEvents()) {
    receiveCode = mip.readIRDongleCode();

    referenceCode++;

    Serial1.printf("Received 0x%02X %02X %02X %02X%s\r\n",
            (receiveCode >> 28) & 0xFF, (receiveCode >> 16) & 0xFF, (receiveCode >> 8) & 0xFF, receiveCode & 0xFF, receiveCode == referenceCode ? "" : " *");

    referenceCode = receiveCode;
  }
}
