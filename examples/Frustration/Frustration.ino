/* Copyright (C) 2019  Samuel Trassare (https://github.com/tiogaplanet)

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

// MiP happily wanders around until it detects too many near obstructions within a minute of its last detected
// obstruction.  Then it gets frustrated.  This sketch is intended to be a "built-in" sketch for MiPs
// containing the MiP D1 mini Pack.

#include <mip_esp8266.h>

static MiP mip;

uint8_t red = 0x00;
uint8_t green = 0x00;
const uint8_t blue = 0x00;                 // We'll just use solid red and solid green in this sketch.

const long cooldownInterval = 60000;       // Interval in which MiP calms down. Default is one minute (60000 ms).
const uint8_t frustrationThreshold = 4;    // The number of obstructions MiP can tolerate within the cool down period before expressing frustration.

uint8_t frustrationLevel = 0;              // Each obstruction within the cool down period increases MiP's frustration.

unsigned long previousMillis = 0;          // Time checks will be implemented using the clock, not the delay function.

void setup() {

  // Initialize the serial connection with the MiP.
  bool connectResult = mip.begin();
  if (!connectResult)
  {
    Serial.println(F("Failed connecting to MiP.  Is it turned on?"));
    return;
  }

  // My preference to start MiP quietly.
  mip.writeVolume(0);

  randomSeed(millis());

  mip.enableRadarMode();
}

void loop() {
  while (mip.isUpright()) {
    mip.continuousDrive(16, 0);

    static MiPRadar lastRadar = MIP_RADAR_INVALID;
    MiPRadar        currentRadar = mip.readRadar();

    unsigned long currentMillis = millis();

    if (currentRadar != MIP_RADAR_INVALID && lastRadar != currentRadar)
    {
      switch (currentRadar)
      {
        case MIP_RADAR_NONE:
          // No obstruction, continue on happily.
          break;
        case MIP_RADAR_10CM_30CM:
          // distant obstruction detected, take evasive maneuvers.
          randomEvasion();
          mip.continuousDrive(16, 0);
          break;
        case MIP_RADAR_0CM_10CM:
          // Near obstruction detected, reset the cool down clock and increase frustration level.
          previousMillis = currentMillis;
          frustrationLevel++;
          if (frustrationLevel != frustrationThreshold) {
            randomEvasion();
            mip.continuousDrive(16, 0);
          }
          break;
        default:
          break;
      }
      lastRadar = currentRadar;
    }

    // This is it.  If MiP has exceeded its frustration threshold, have a good ol' tantrum and go back to normal.
    if (frustrationLevel >= frustrationThreshold) {
      frustration();
      previousMillis = currentMillis;
      frustrationLevel = 0;
    } else if (currentMillis - previousMillis >= cooldownInterval) { // Otherwise, if MiP has avoided near obstacles for
      previousMillis = currentMillis;                                // the last minute, reset the frustration level.
      frustrationLevel = 0;
    }
  }

  // If MiP detects a clap in any position other than upright, this sketch ends and relinquishes control back to
  // factory programming.
  while (!mip.isUpright()) {
    mip.disableRadarMode();
    mip.enableClapEvents();
    if (mip.availableClapEvents() > 0) {
      mip.writeHeadLEDs(MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF, MIP_HEAD_LED_OFF);
      mip.end();
    }
  }

  mip.enableRadarMode();
}

void frustration() {

  // Set the chest LED to red.
  red = 0xFF;
  green = 0x00;
  mip.writeChestLED(red, green, blue);

  //Make an angry noise.
  mip.beginSoundList();
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_4, 0);
  mip.addEntryToSoundList(MIP_SOUND_MOOD_ANGRY, 1000);
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_OFF, 0);
  mip.playSoundList(0);

  // flash the eyes agrily.
  MiPHeadLEDs headLEDs;
  headLEDs.led2 = headLEDs.led3 = MIP_HEAD_LED_BLINK_FAST;
  headLEDs.led1 = headLEDs.led4 = MIP_HEAD_LED_BLINK_SLOW;
  mip.writeHeadLEDs(headLEDs);

  // Do three spins, each in a random direction for a random number of degrees at max speed, of course.
  for (uint8_t i = 0; i < 3; i++) {
    (random(0, 2)) ? mip.turnLeft(random(0, 1276), 24) : mip.turnRight(random(0, 1276), 24);
    delay(1500);
  }

  // restore the eyes to normal.
  headLEDs.led1 = headLEDs.led2 = headLEDs.led3 = headLEDs.led4 = MIP_HEAD_LED_ON;
  mip.writeHeadLEDs(headLEDs);

  // Make "exhaustion" noise.
  mip.beginSoundList();
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_4, 0);
  mip.addEntryToSoundList(MIP_SOUND_ACTION_OUT_OF_BREATH, 0);
  mip.addEntryToSoundList(MIP_SOUND_VOLUME_OFF, 0);
  mip.playSoundList(0);

  // Set the chest LED back to green and get on with life.
  red = 0x00;
  green = 0xFF;
  mip.writeChestLED(red, green, blue);
}

void randomEvasion() {
  // randomly turn right or left to avoid obstruction.
  (random(0, 2) == 0) ? mip.turnLeft(90, 12) : mip.turnRight(90, 12);
  delay(500);
}
