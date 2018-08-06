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
    readSoftwareVersion()
    readHardwareInfo()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("SoftwareHardwareVersion.ino - Use readSoftwareVersion() & readHardwareInfo() functions."));

  MiPSoftwareVersion softwareVersion;
  mip.readSoftwareVersion(softwareVersion);
  Serial1.print(F("software version: "));
  Serial1.print(softwareVersion.year);
    Serial1.print('-');
    Serial1.print(softwareVersion.month);
    Serial1.print('-');
    Serial1.print(softwareVersion.day);
    Serial1.print('.');
    Serial1.println(softwareVersion.uniqueVersion);

  MiPHardwareInfo hardwareInfo;
  mip.readHardwareInfo(hardwareInfo);
  Serial1.println(F("hardware info"));
  Serial1.print(F("  voice chip version: "));
    Serial1.println(hardwareInfo.voiceChip);
  Serial1.print(F("  hardware version: "));
    Serial1.println(hardwareInfo.hardware);

  Serial1.println();
  Serial1.println(F("Sample done."));
}

void loop() {
}

