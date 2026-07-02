/* Copyright (C) 2026  Samuel Trassare (https://github.com/Tiogaplanet)

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
/* This implementation file provides access to audio functions of the WowWee MiP
   self-balancing robot.
*/
#include "MPU_D1_mini.h"

#define MIP_MAX_RETRIES 2
#define MIP_RETRY_WAIT 50

// MiP Protocol Commands related to audio playback.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_PLAY_SOUND 0x06
#define MIP_CMD_SET_VOLUME 0x15
#define MIP_CMD_GET_VOLUME 0x16

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber) {
  MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_Audio.cpp: %d\n", lineNumber);
  while (1) { delay(100); }
}

void MiP::playSound(MiPSoundIndex sound, MiPVolume volume /* = MIP_VOLUME_DEFAULT */) {
  beginSoundList();
  addEntryToSoundList(sound, 0, volume);
  playSoundList();
}

void MiP::beginSoundList() {
  m_soundIndex = 0;
  m_playVolume = MIP_VOLUME_DEFAULT;
  m_lastError = MIP_ERROR_NONE;
}

void MiP::addEntryToSoundList(MiPSoundIndex sound, uint16_t delayTime /* = 0 */, MiPVolume volume /* = MIP_VOLUME_DEFAULT */) {
  // Must call beginSoundList() before calling this function.
  MIP_ASSERT(m_soundIndex != -1);

  // Delay is in units of 30 msecs and can't exceed 255 * 30.
  MIP_ASSERT(delayTime <= 255 * 30);

  // Volume can only be set to values between 0 and 7 or 0xFF (which means keep volume as it was).
  MIP_ASSERT(volume <= MIP_VOLUME_7 || volume == MIP_VOLUME_DEFAULT);

  // Need to issue volume command if volume is being changed and
  // if we have to inject a volume instruction, verify we don't overflow the buffer.
  if (volume != MIP_VOLUME_DEFAULT && volume != m_playVolume) {
    // Safe check to prevent index 18 out-of-bounds write.
    // The sound list can only hold 8 sound entries.
    MIP_ASSERT(m_soundIndex < 8);
    m_playCommand[1 + m_soundIndex * 2] = MIP_SOUND_VOLUME_OFF + volume;
    m_playCommand[1 + m_soundIndex * 2 + 1] = 0;
    m_playVolume = volume;
    m_soundIndex++;
  }

  // The sound list can only hold 8 sound entries.
  MIP_ASSERT(m_soundIndex < 8);
  m_playCommand[1 + m_soundIndex * 2] = sound;
  m_playCommand[1 + m_soundIndex * 2 + 1] = delayTime / 30;
  m_soundIndex++;

  m_lastError = MIP_ERROR_NONE;
}

void MiP::playSoundList(uint8_t repeatCount) {
  // Must call beginSoundList() and addSoundToList() before calling this function.
  MIP_ASSERT(m_soundIndex >= 1);
  m_playCommand[0] = MIP_CMD_PLAY_SOUND;

  // Fill out the rest of the command buffer with mute sounds.
  while (m_soundIndex < 8) {
    m_playCommand[1 + m_soundIndex * 2] = MIP_SOUND_SHORT_MUTE_FOR_STOP;
    m_playCommand[1 + m_soundIndex * 2 + 1] = 0;
    m_soundIndex++;
  }

  // The last byte in the command is the repeat count.
  m_playCommand[17] = repeatCount;

  // Send this command blindly with no error checking since there is no way to determine if it has failed.
  rawSend(m_playCommand, sizeof(m_playCommand));

  // Set the index to 8 to flag that no more items can be added to the sound list but you can still play it again.
  m_soundIndex = 8;
  m_lastError = MIP_ERROR_NONE;
}

void MiP::writeVolume(uint8_t volume) {
  int8_t result;

  // Send the set command and then issue the corresponding get command. Retry if the get fails or doesn't return the
  // expected new setting.
  for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++) {
    rawSetVolume(volume);

    // Read back and make sure that it was set as expected.
    uint8_t updatedVolume;
    result = rawGetVolume(updatedVolume);
    if (result == MIP_ERROR_NONE && updatedVolume == volume) {
      // The set was successful so return immediately.
      m_lastError = MIP_ERROR_NONE;
      return;
    }

    // An error was encountered so we will loop around and try again.
    // Wait for a bit before the next retry.
    delay(MIP_RETRY_WAIT);
  }

  if (result != MIP_ERROR_NONE) {
    // Kept getting an error back from read attempt.
    m_lastError = result;
  } else {
    // Read was successful but didn't match setting to which we were attempting to change.
    m_lastError = MIP_ERROR_MAX_RETRIES;
  }
}

uint8_t MiP::readVolume() {
  int8_t result;

  // Retry the read if it should fail on the first attempt.
  for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++) {
    uint8_t volume;
    result = rawGetVolume(volume);
    if (result == MIP_ERROR_NONE) {
      m_lastError = MIP_ERROR_NONE;
      return volume;
    }

    // An error was encountered so we will loop around and try again.
    // Wait for a bit before the next retry.
    delay(MIP_RETRY_WAIT);
  }

  m_lastError = result;
  return 0;
}

// This internal protected method sends the set volume command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetVolume(uint8_t volume) {
  MIP_ASSERT(volume <= 7);
  uint8_t command[1 + 1] = { MIP_CMD_SET_VOLUME, volume };
  rawSend(command, sizeof(command));
}

// This internal protected method sends the get volume command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetVolume(uint8_t& volume) {
  const uint8_t getVolume[1] = { MIP_CMD_GET_VOLUME };
  uint8_t       response[1 + 1];
  size_t        responseLength;
  volume = 0;
  int8_t result = rawReceive(getVolume, sizeof(getVolume), response, sizeof(response), responseLength);
  if (result) return result;
  if (responseLength != sizeof(response) ||
      response[0] != MIP_CMD_GET_VOLUME ||
      response[1] > 7) {
    return MIP_ERROR_BAD_RESPONSE;
  }
  volume = response[1];
  return result;
}
