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
/* Demonstrates use of the SPI flash file system (SPIFFS) on the ESP8266.
*/
#include <mip_esp8266.h>
#include "FS.h"

MiP     mip;

void setup() {
  String password = "1234secret";

  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("SPIFFS.ino - Read and write the the SPI flash file system (SPIFFS).\n"));
  
  Serial1.println(F("Chest turns violet if the read matches the write, else red.\n"));

  // The call to begin() mounts the flash filesystem.
  (SPIFFS.begin())  ? Serial1.println(F("SPIFFS opened.")) : Serial1.println(F("\n\nSPIFFS failed to open."));

  // Open the file in write mode.
  File f = SPIFFS.open("/f.txt", "w");
  if (!f) {
    Serial1.println(F("File creation failed."));
  }
  // Write a "password."
  f.println(password);
  f.close();

  // Open the file "f.txt" in read mode.
  f = SPIFFS.open("/f.txt", "r");

  String line;

  // Read from the file.
  while (f.available()) {
    line = f.readStringUntil('\n');
  }
  
  line.trim();
  
  Serial1.println("Password is " + password + ".");
  Serial1.println("File contained " + line + ".");
  
  (line == password) ? mip.writeChestLED(0xB6, 0x00, 0xFF) : mip.writeChestLED(0xFF, 0x00, 0x00);
  
  f.close();

  (SPIFFS.remove("/f.txt")) ? Serial1.println(F("File deleted.")) : Serial1.println(F("Error deleting file."));

  delay(5000);
  mip.writeChestLED(0x00, 0xFF, 0x00);
}

void loop() {
}

