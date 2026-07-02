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
/* This header file describes the public API that an application can use to communicate with the WowWee MiP
   self-balancing robot.
*/
#ifndef MPU_D1_MINI_H
#define MPU_D1_MINI_H

#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include "MPU_Queue.h"
#include "MPU_Types.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


// Setup some debug levels for reporting library status via Serial1.
#define MIP_DEBUG_NONE 0
#define MIP_DEBUG_ERROR 1
#define MIP_DEBUG_WARN 2
#define MIP_DEBUG_INFO 3

// Default to NONE if not defined by the user in the sketch.
#ifndef MIP_DEBUG_LEVEL
  #define MIP_DEBUG_LEVEL MIP_DEBUG_INFO
#endif

// Create the macros for conditional printing of debug messages via Serial1.
#if MIP_DEBUG_LEVEL >= MIP_DEBUG_ERROR
  #define MIP_DEBUG_ERROR_PRINT(...)   Serial1.print(F("[ERROR] ")); Serial1.print(__VA_ARGS__)
  #define MIP_DEBUG_ERROR_PRINTLN(...) Serial1.print(F("[ERROR] ")); Serial1.println(__VA_ARGS__)
  #define MIP_DEBUG_ERROR_PRINTF(...)  Serial1.print(F("[ERROR] ")); Serial1.printf(__VA_ARGS__)
#else
  #define MIP_DEBUG_ERROR_PRINT(...)
  #define MIP_DEBUG_ERROR_PRINTLN(...)
  #define MIP_DEBUG_ERROR_PRINTF(...)
#endif
#if MIP_DEBUG_LEVEL >= MIP_DEBUG_WARN
  #define MIP_DEBUG_WARN_PRINT(...)   Serial1.print(F("[WARN] ")); Serial1.print(__VA_ARGS__)
  #define MIP_DEBUG_WARN_PRINTLN(...) Serial1.print(F("[WARN] ")); Serial1.println(__VA_ARGS__)
  #define MIP_DEBUG_WARN_PRINTF(...)  Serial1.print(F("[WARN] ")); Serial1.printf(__VA_ARGS__)
#else
  #define MIP_DEBUG_WARN_PRINT(...)
  #define MIP_DEBUG_WARN_PRINTLN(...)
  #define MIP_DEBUG_WARN_PRINTF(...)
#endif
#if MIP_DEBUG_LEVEL >= MIP_DEBUG_INFO
  #define MIP_DEBUG_INFO_PRINT(...)   Serial1.print(F("[INFO] ")); Serial1.print(__VA_ARGS__)
  #define MIP_DEBUG_INFO_PRINTLN(...) Serial1.print(F("[INFO] ")); Serial1.println(__VA_ARGS__)
  #define MIP_DEBUG_INFO_PRINTF(...)  Serial1.print(F("[INFO] ")); Serial1.printf(__VA_ARGS__)
#else
  #define MIP_DEBUG_INFO_PRINT(...)
  #define MIP_DEBUG_INFO_PRINTLN(...)
  #define MIP_DEBUG_INFO_PRINTF(...)
#endif

// Integer error codes that can be encountered by MiP API functions.
#define MIP_ERROR_NONE          0 // Success
#define MIP_ERROR_TIMEOUT       1 // Timed out waiting for response.
#define MIP_ERROR_NO_EVENT      2 // No event has arrived from MiP yet.
#define MIP_ERROR_BAD_RESPONSE  3 // Unexpected response from MiP.
#define MIP_ERROR_MAX_RETRIES   4 // Exceeded maximum number of retries to get this operation to succeed.

// Maximum length of MiP request and response buffer lengths.
#define MIP_REQUEST_MAX_LEN     (17 + 1)    // Longest request is MIP_CMD_PLAY_SOUND.
#define MIP_RESPONSE_MAX_LEN    (5 + 1)     // Longest response is MIP_CMD_REQUEST_CHEST_LED.

class MiP {
public:
  // EEPROM base address.  When reading or writing to EEPROM the user will pass an offset that is added to this base address.
  static constexpr uint8_t BASE_EEPROM_ADDRESS = 0x20;
  // Last addressable address in EEPROM.
  static constexpr uint8_t LAST_EEPROM_ADDRESS = 0x2F;

  // Constructor/Destructors.
  MiP();
  ~MiP();

  bool begin();
  bool begin(const char* ssid, const char* password, const char* hostname);
  void end();
  void sleep();

  // Will return false if begin() wasn't successful in connecting to MiP.
  bool isInitialized() {
    return (m_flags & MRI_FLAG_INITIALIZED);
  }

  // When calling the public functions listed below, the MiP library will try its best to handle any errors
  // encountered by retrying the read/write operations behind the scenes. If the worst happens and it just can't
  // recover from a communication issue with MiP, it will provide details about the cause of the problem through
  // the following functions.
  int8_t lastCallResult() {
    return m_lastError;
  }
  bool didLastCallFail() {
    return m_lastError != MIP_ERROR_NONE;
  }
  void printLastCallResult();

  // Sensors, Gestures, Radar
  void enableRadarMode();
  void disableRadarMode();
  void enableGestureMode();
  void disableGestureMode();
  bool isRadarModeEnabled();
  bool isGestureModeEnabled();
  bool areGestureAndRadarModesDisabled();
  MiPRadar readRadar();
  uint8_t availableGestureEvents();
  MiPGesture readGestureEvent();

  // LEDs
  void writeChestLED(uint8_t red, uint8_t green, uint8_t blue);

  void writeChestLED(uint8_t red, uint8_t green, uint8_t blue, uint16_t onTime, uint16_t offTime);
  void writeChestLED(const MiPChestLED& chestLED);
  void readChestLED(MiPChestLED& chestLED);
  void unverifiedWriteChestLED(uint8_t red, uint8_t green, uint8_t blue);
  void unverifiedWriteChestLED(uint8_t red, uint8_t green, uint8_t blue, uint16_t onTime, uint16_t offTime);
  void unverifiedWriteChestLED(const MiPChestLED& chestLED);

  void writeHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4);
  void writeHeadLEDs(const MiPHeadLEDs& headLEDs);
  void readHeadLEDs(MiPHeadLEDs& headLEDs);
  void unverifiedWriteHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4);
  void unverifiedWriteHeadLEDs(const MiPHeadLEDs& headLEDs);

  // Motion
  void continuousDrive(int8_t velocity, int8_t turnRate);
  void distanceDrive(MiPDriveDirection driveDirection, uint8_t cm, MiPTurnDirection turnDirection, uint16_t degrees);
  void turnLeft(uint16_t degrees, uint8_t speed);
  void turnRight(uint16_t degrees, uint8_t speed);
  void driveForward(uint8_t speed, uint16_t time);
  void driveBackward(uint8_t speed, uint16_t time);
  void stop();
  void fallForward();
  void fallBackward();
  void getUp(MiPGetUp getup = MIP_GETUP_FROM_EITHER);

  // Audio & Volume
  void playSound(MiPSoundIndex sound, MiPVolume volume = MIP_VOLUME_DEFAULT);

  void beginSoundList();
  void addEntryToSoundList(MiPSoundIndex sound, uint16_t delay = 0, MiPVolume volume = MIP_VOLUME_DEFAULT);
  void playSoundList(uint8_t repeatCount = 0);

  void writeVolume(uint8_t volume);
  uint8_t readVolume();

  // Odometer & Status
  float readDistanceTravelled();
  void  resetDistanceTravelled();

  float readBatteryVoltage();
  MiPPosition readPosition();
  bool  isOnBack();
  bool  isFaceDown();
  bool  isUpright();
  bool  isPickedUp();
  bool  isHandStanding();
  bool  isFaceDownOnTray();
  bool  isOnBackWithKickstand();

  int8_t readWeight();

  // Claps & Shaking
  void     enableClapEvents();
  void     disableClapEvents();
  bool     areClapEventsEnabled();
  void     writeClapDelay(uint16_t delay);
  uint16_t readClapDelay();
  uint8_t  availableClapEvents();
  uint8_t  readClapEvent();
  bool     hasBeenShaken();

  // Device Info
  void readSoftwareVersion(MiPSoftwareVersion& software);
  void readHardwareInfo(MiPHardwareInfo& hardware);

  // Game Modes
  void enableAppMode();
  void enableCageMode();
  void enableDanceMode();
  void enableStackMode();
  void enableTrickMode();
  void enableRoamMode();
  bool isAppModeEnabled();
  bool isCageModeEnabled();
  bool isDanceModeEnabled();
  bool isStackModeEnabled();
  bool isTrickModeEnabled();
  bool isRoamModeEnabled();

  // EEPROM User Data
  void    setUserData(uint8_t addressOffset, uint8_t userData);
  uint8_t getUserData(uint8_t addressOffset);

  // IR & Detection
  void     enableMiPDetectionMode(uint8_t id, uint8_t txPower);
  void     disableMiPDetectionMode();
  bool     isMiPDetectionModeEnabled();
  uint8_t  readDetectedMiP();
  uint8_t  availableDetectedMiPEvents();
  void     enableIRRemoteControl();
  void     disableIRRemoteControl();
  bool     isIRRemoteControlEnabled();
  void     sendIRDongleCode(uint16_t sendCode, uint8_t transmitPower);
  uint32_t readIRDongleCode();
  uint8_t  availableIRCodeEvents();

  // Lower Level Raw API
  void   rawSend(const uint8_t request[], size_t requestLength);
  int8_t rawReceive(const uint8_t request[], size_t requestLength,
                    uint8_t responseBuffer[], size_t responseBufferSize, size_t& responseLength);

protected:
  void    clear();
  int8_t  attemptMiPConnection(uint32_t baudRate);

  void    connect();

  // Helper utilities for sub-functions
  void    verifiedSetGestureRadarMode(MiPGestureRadarMode desiredMode);
  bool    checkGestureRadarMode(MiPGestureRadarMode expectedMode);
  void    rawSetGestureRadarMode(MiPGestureRadarMode mode);
  int8_t  rawGetGestureRadarMode(MiPGestureRadarMode& mode);

  void    rawSetChestLED(uint8_t red, uint8_t green, uint8_t blue);
  void    rawFlashChestLED(uint8_t red, uint8_t green, uint8_t blue, uint8_t onTime, uint8_t offTime);
  int8_t  rawGetChestLED(MiPChestLED& chestLED);

  void    rawSetHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4);
  int8_t  rawGetHeadLEDs(MiPHeadLEDs& headLEDs);
  bool    isValidHeadLED(uint8_t led);

  void    fallDown(MiPFallDirection direction);

  void    rawSetVolume(uint8_t volume);
  int8_t  rawGetVolume(uint8_t& volume);

  int8_t  rawReadOdometer(float& distanceInCm);

  int8_t  rawGetStatus(MiPStatus& status);
  int8_t  parseStatus(MiPStatus& status, const uint8_t response[], size_t responseLength);

  int8_t  rawGetWeight(int8_t& weight);
  int8_t  parseWeight(int8_t& weight, const uint8_t response[], size_t responseLength);

  void    checkedEnableClapEvents(MiPClapEnabled enabled);
  int8_t  readClapSettings(MiPClapSettings& settings);
  void    rawEnableClap(MiPClapEnabled enabled);
  void    rawSetClapDelay(uint16_t delay);
  int8_t  rawGetClapSettings(MiPClapSettings& settings);

  int8_t  rawGetSoftwareVersion(MiPSoftwareVersion& software);
  int8_t  rawGetHardwareInfo(MiPHardwareInfo& hardware);

  void    verifiedSetGameMode(MiPGameMode desiredMode);
  bool    checkGameMode(MiPGameMode expectedMode);
  void    rawSetGameMode(MiPGameMode mode);
  int8_t  rawGetGameMode(MiPGameMode& mode);

  void    rawSetUserData(uint8_t address, uint8_t userData);
  int8_t  rawGetUserData(uint8_t address, uint8_t& userData);

  void    rawSetMiPDetectionMode(uint8_t id, uint8_t txPower);
  void    verifiedIRRemoteControl(uint8_t desiredRemoteControlMode);
  void    rawSetIRRemoteControl(uint8_t remoteControl);
  int8_t  rawGetIRRemoteControl(uint8_t& remoteControl);

  // Transport level logic
  void    transportSendRequest(const uint8_t* pRequest, size_t requestLength, int expectResponse);
  int8_t  transportGetResponse(uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength);
  bool    processAllResponseData();
  void    copyHexTextToBinary(uint8_t* pDest, uint8_t* pSrc, uint8_t length);
  uint8_t parseHexDigit(uint8_t digit);
  void    processOobResponseData(uint8_t commandByte);
  uint8_t discardUnexpectedSerialData();

  // Bits that can be set in m_flags bitfield.
  enum FlagBits : uint8_t {
    MIP_FLAG_RADAR_VALID     = (1 << 0),
    MIP_FLAG_SHAKE_DETECTED  = (1 << 1),
    MIP_FLAG_WEIGHT_VALID    = (1 << 2),
    MRI_FLAG_INITIALIZED     = (1 << 3)
  };

  uint32_t                     m_lastRequestTime;
  uint32_t                     m_lastContinuousDriveTime;
  uint8_t                      m_flags;
  uint8_t                      m_responseBuffer[MIP_RESPONSE_MAX_LEN];
  uint8_t                      m_expectedResponseCommand;
  uint8_t                      m_expectedResponseSize;
  int8_t                       m_lastError;
  uint8_t                      m_playCommand[1 + 18]; // Gemini Enterprise: Increased by 1 to prevent out-of-bounds write!
  int8_t                       m_soundIndex;
  uint8_t                      m_playVolume;
  MiPRadar                     m_lastRadar;
  MiPStatus                    m_lastStatus;
  int8_t                       m_lastWeight;
  CircularQueue<uint8_t, 8>    m_clapEvents;
  CircularQueue<MiPGesture, 8> m_gestureEvents;
  CircularQueue<uint32_t, 8>   m_irCodeEvents;
  CircularQueue<uint8_t, 8>    m_detectedMiPEvents;
  uint8_t                      m_irId;
  char                         m_ssid[32];
  char                         m_password[64];
  char                         m_hostname[63];
};

#endif // MPU_D1_MINI_H
