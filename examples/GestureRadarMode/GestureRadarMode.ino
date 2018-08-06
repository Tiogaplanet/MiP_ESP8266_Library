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
    enableRadarMode();
    disableRadarMode();
    enableGestureMode();
    disableGestureMode();
    isRadarModeEnabled();
    isGestureModeEnabled();
    isGestureAndRadarModeDisabled();
*/
#include <mip_esp8266.h>

MiP     mip;

void setup() {
  bool connectResult = mip.begin();
  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP!"));
    return;
  }

  Serial1.println(F("GestureRadarMode.ino - Switches between gesture, radar, and default modes."));

  Serial1.println(F("Calling mip.enableRadarMode()"));
  mip.enableRadarMode();
  Serial1.print(F("mip.isRadarModeEnabled() = "));
  if (mip.isRadarModeEnabled()) {
    Serial1.println(F("true - Pass"));
  } else {
    Serial1.println(F("false - Failed"));
  }

  Serial1.println(F("Calling mip.disableRadarMode()"));
  mip.disableRadarMode();
  Serial1.print(F("mip.isRadarModeEnabled() = "));
  if (mip.isRadarModeEnabled()) {
    Serial1.println(F("true - Failed"));
  } else {
    Serial1.println(F("false - Pass"));
  }

  Serial1.println(F("Calling mip.enableGestureMode()"));
  mip.enableGestureMode();
  Serial1.print(F("mip.isGestureModeEnabled() = "));
  if (mip.isGestureModeEnabled()) {
    Serial1.println(F("true - Pass"));
  } else {
    Serial1.println(F("false - Failed"));
  }

  Serial1.println(F("Calling mip.disableGestureMode()"));
  mip.disableGestureMode();
  Serial1.print(F("mip.isGestureModeEnabled() = "));
  if (mip.isGestureModeEnabled()) {
    Serial1.println(F("true - Failed"));
  } else {
    Serial1.println(F("false - Pass"));
  }
  Serial1.print(F("mip.areGestureAndRadarModesDisabled() = "));
  if (mip.areGestureAndRadarModesDisabled()) {
    Serial1.println(F("true - Pass"));
  } else {
    Serial1.println(F("false - Failed"));
  }

  Serial1.println();
  Serial1.println(F("Sample done."));
}

void loop() {
}

