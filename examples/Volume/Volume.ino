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
    writeVolume()
    readVolume()
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("Volume.ino - Use read/writeVolume(). Set volume level to 1 and read out afterwards."));

  mip.writeVolume(MIP_VOLUME_OFF);

  uint8_t volume = mip.readVolume();

  Serial1.print(F("Volume = "));
    Serial1.println(volume);

  Serial1.println();
  Serial1.println(F("Sample done."));
}

void loop() {
}
