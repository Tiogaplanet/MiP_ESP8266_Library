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
/* Contains the constructor, destructor and basic connection initialization.
*/
#include "MPU_D1_mini.h"

// Number of times that begin() method should try to initialize the MiP.
#define MIP_MAX_BEGIN_RETRIES 5

// Number of milliseconds to wait between retries in begin().
#define MIP_BEGIN_RETRY_WAIT 500

// Baud rate used for the ESP8266 debug channel.
#define ESP8266_DEBUG_BAUD_RATE 74880

// Fast baud rate for MiP communications.
#define MIP_FAST_BAUD_RATE 115200

// Slow baud rate for MiP communications.  MiPs support one or the other.
#define MIP_SLOW_BAUD_RATE 9600

// MiP Protocol Commands related to core functions.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_DISCONNECT_APP 0xFE
#define MIP_CMD_SLEEP 0xFA

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber) {
  MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_Core.cpp: %d\n", lineNumber);
  while (1) { delay(100); }
}

MiP::MiP() { clear(); }
MiP::~MiP() { end(); }

void MiP::clear() {
  m_lastRequestTime = millis();
  m_lastContinuousDriveTime = millis();
  m_flags = 0;
  memset(m_responseBuffer, 0, sizeof(m_responseBuffer));
  m_expectedResponseCommand = 0;
  m_expectedResponseSize = 0;
  m_lastError = MIP_ERROR_NONE;
  memset(m_playCommand, 0, sizeof(m_playCommand));
  m_soundIndex = -1;
  m_playVolume = MIP_VOLUME_OFF;
  m_lastRadar = MIP_RADAR_INVALID;
  m_lastStatus.clear();
  m_lastWeight = 0;
  m_clapEvents.clear();
  m_gestureEvents.clear();
  m_detectedMiPEvents.clear();
  m_irCodeEvents.clear();
  m_irId = 0x00;
  memset(m_ssid, 0, sizeof(m_ssid));
  memset(m_password, 0, sizeof(m_password));
  memset(m_hostname, 0, sizeof(m_hostname));
}

bool MiP::begin() {
  // Setup the debugging channel.
  Serial1.begin(ESP8266_DEBUG_BAUD_RATE);

  // Initialize the class members.
  clear();

  // Roll the timers back so that the first calls can occur immediately.
  m_lastRequestTime = millis() - 10; // MIP_REQUEST_DELAY; // (10) offset slightly
  m_lastContinuousDriveTime = millis() - 50; // MIP_CONTINUOUS_DRIVE_DELAY; // (50)

  // Assume that the connection to MiP will be successfully initialized. Will clear the flag if a connection
  // error is detected. If this wasn't done then the calls to rawSend() and rawGetStatus() below would fail.
  m_flags |= MRI_FLAG_INITIALIZED;

  // Sometimes the init fails. It seems to happen when the MiP is busy at power-up doing other things like
  // attempting to balance.
  int8_t retry;
  for (retry = 0 ; retry < MIP_MAX_BEGIN_RETRIES ; retry++) {
    // Try to connect at 115200 baud, the rate used by some MiPs.
    int8_t result = attemptMiPConnection(MIP_FAST_BAUD_RATE);
    if (result == MIP_ERROR_NONE) return true;

    // Try to connect at 9600 baud if the fast attempt failed.
    result = attemptMiPConnection(MIP_SLOW_BAUD_RATE);
    if (result == MIP_ERROR_NONE) return true;
  }

  // Get here if the connection attempt to MiP never succeeds.
  m_flags &= ~MRI_FLAG_INITIALIZED;
  end();
  return false;
}

// This internal protected method provides the common code for connection attempts at
// baud rates of 115200 or 9600.
int8_t MiP::attemptMiPConnection(uint32_t baudRate) {
  // Set baud rate to specified rate.
  Serial.begin(baudRate);

  // Swap the UART of the D1 mini to the alternate pins.
  Serial.swap();

  // Send 0xFF to the MiP via UART to enable the UART communication channel in the MiP.
  const uint8_t initMipCommand[] = { 0xFF };
  rawSend(initMipCommand, sizeof(initMipCommand));

  // The MiP UART documentation indicates that this delay is required after sending 0xFF.
  delay(30);

  // Flush any outstanding junk data in receive buffer.
  discardUnexpectedSerialData();

  // Attempt to get MiP's latest status to see if the connection was successful or not.
  int8_t result = rawGetStatus(m_lastStatus);
  if (result == MIP_ERROR_NONE) {
    // Let the user know at which baud rate the connection to MiP was made.
    MIP_DEBUG_INFO_PRINTF("MiP: Connected at %d baud\n\r", baudRate);
  } else {
    // Sleep a bit before returning to code which will retry connection at alternate baud rate.
    delay(MIP_BEGIN_RETRY_WAIT);
  }
  return result;
}

void MiP::end() {
  if (isInitialized()) {
    // Restore MiP's default volume in case it was changed by the user.
    writeVolume(MIP_VOLUME_7);

    // Send the disconnect command.  If it is successful the app will be disconnected, indicated by a
    // blue chest LED.
    const uint8_t command[] = { MIP_CMD_DISCONNECT_APP };
    rawSend(command, sizeof(command));
  }

  clear();

  // Swap the UART on the D1 mini back to the default RX/TX pair.
  Serial.swap();
  Serial.end();

  // Shutdown the debugging channel.
  Serial1.end();
}

void MiP::sleep() {
  // Put the MiP to sleep.
  // The MiP will need to be reset before another begin() will succeed.
  const uint8_t command[] = { MIP_CMD_SLEEP };
  rawSend(command, sizeof(command));
}

void MiP::printLastCallResult() {
  if (m_lastError != MIP_ERROR_NONE) {
    MIP_DEBUG_ERROR_PRINT(F("MiP: API returned "));
    switch (m_lastError) {
      case MIP_ERROR_TIMEOUT:
        MIP_DEBUG_ERROR_PRINTLN(F("MIP_ERROR_TIMEOUT (Timed out waiting for response)"));
        break;
      case MIP_ERROR_NO_EVENT:
        MIP_DEBUG_ERROR_PRINTLN(F("MIP_ERROR_NO_EVENT (No event has arrived from MiP yet)"));
        break;
      case MIP_ERROR_BAD_RESPONSE:
        MIP_DEBUG_ERROR_PRINTLN(F("MIP_ERROR_BAD_RESPONSE (Unexpected response from MiP)"));
        break;
      case MIP_ERROR_MAX_RETRIES:
        MIP_DEBUG_ERROR_PRINTLN(F("MIP_ERROR_MAX_RETRIES (Exceeded maximum number of retries)"));
        break;
      default:
        MIP_DEBUG_ERROR_PRINTLN(F("unknown error"));
        break;
    }
  }
}
