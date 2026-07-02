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
/* This file implements decoding and fetching for all radar tracking steps, gesture inputs,
   claps, shaking events, physical weight sensors, battery voltage metrics, and IR dongle 
   communication.
*/
#include "MPU_D1_mini.h"

#define MIP_MAX_RETRIES 2
#define MIP_RETRY_WAIT 50

// MiP Protocol Commands related to sensors.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_GET_GESTURE_RADAR_MODE  0x0D
#define MIP_CMD_SET_GESTURE_RADAR_MODE  0x0C
#define MIP_CMD_READ_ODOMETER           0x85
#define MIP_CMD_RESET_ODOMETER          0x86
#define MIP_CMD_GET_STATUS              0x79
#define MIP_CMD_GET_WEIGHT              0x81
#define MIP_CMD_ENABLE_CLAP             0x1E
#define MIP_CMD_SET_CLAP_DELAY          0x20
#define MIP_CMD_GET_CLAP_SETTINGS       0x1F
#define MIP_CMD_GET_SOFTWARE_VERSION    0x14
#define MIP_CMD_GET_HARDWARE_INFO       0x19
#define MIP_CMD_SET_GAME_MODE           0x76
#define MIP_CMD_GET_GAME_MODE           0x82
#define MIP_CMD_SET_USER_DATA           0x12
#define MIP_CMD_GET_USER_DATA           0x13
#define MIP_CMD_SET_DETECTION_MODE      0x0E
#define MIP_CMD_SET_IR_REMOTE_CONTROL   0x10
#define MIP_CMD_GET_IR_REMOTE_CONTROL   0x11
#define MIP_CMD_SEND_IR_DONGLE_CODE     0x8C

// IR mode definitions.
#define MIP_IR_DETECTION_MODE_DISABLE 0
#define MIP_IR_REMOTE_CONTROL_DISABLE 0
#define MIP_IR_REMOTE_CONTROL_ENABLE  1

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber)
{
    MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_Sensors.cpp: %d\n", lineNumber);
    while (1) { delay(100); }
}

void MiP::enableRadarMode() { verifiedSetGestureRadarMode(MIP_RADAR); }
void MiP::enableGestureMode() { verifiedSetGestureRadarMode(MIP_GESTURE); }
void MiP::disableRadarMode() { verifiedSetGestureRadarMode(MIP_GESTURE_RADAR_DISABLED); }
void MiP::disableGestureMode() { verifiedSetGestureRadarMode(MIP_GESTURE_RADAR_DISABLED); }

// This internal protected method sends the command to change the radar/gesture mode and then sends a request to get
// the new state. If this request fails or the new state isn't as expected, it will retry the command.
void MiP::verifiedSetGestureRadarMode(MiPGestureRadarMode desiredMode)
{
    int8_t result;
	
	// Always mark cached RADAR data as invalid when changing modes.
	
    m_flags &= ~MIP_FLAG_RADAR_VALID;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetGestureRadarMode(desiredMode);
		
		// Read back and make sure that it was set as expected.
        MiPGestureRadarMode actualMode = MIP_GESTURE_RADAR_DISABLED;
        result = rawGetGestureRadarMode(actualMode);
        if (result == MIP_ERROR_NONE && actualMode == desiredMode)
        {
			// The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }
		
		// An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
	
    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from rawGetGestureRadarMode().
        m_lastError = result;
    }
    else
    {
        // rawGetGestureRadarMode() was successful but didn't match mode to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

bool MiP::isRadarModeEnabled() { return checkGestureRadarMode(MIP_RADAR); }
bool MiP::isGestureModeEnabled() { return checkGestureRadarMode(MIP_GESTURE); }
bool MiP::areGestureAndRadarModesDisabled() { return checkGestureRadarMode(MIP_GESTURE_RADAR_DISABLED); }

// This internal protected method requests the current radar/gesture mode and then returns whether it matches the
// passed in value or not. It includes retry code incase the request should fail.
bool MiP::checkGestureRadarMode(MiPGestureRadarMode expectedMode)
{
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        MiPGestureRadarMode currentMode;
        result = rawGetGestureRadarMode(currentMode);
        if (result == MIP_ERROR_NONE) return currentMode == expectedMode;
		
		// An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
    return false;
}

MiPRadar MiP::readRadar()
{
	// Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    if ((m_flags & MIP_FLAG_RADAR_VALID) == 0)
    {
		// Haven't received a radar event yet.
        m_lastError = MIP_ERROR_NO_EVENT;
        return MIP_RADAR_INVALID;
    }
    m_lastError = MIP_ERROR_NONE;
    return m_lastRadar;
}

uint8_t MiP::availableGestureEvents()
{
	// Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    m_lastError = MIP_ERROR_NONE;
    return m_gestureEvents.available();
}

MiPGesture MiP::readGestureEvent()
{
    processAllResponseData();
    MiPGesture gestureEvent = MIP_GESTURE_INVALID;
    if (!m_gestureEvents.pop(gestureEvent))
    {
        m_lastError = MIP_ERROR_NO_EVENT;
        return MIP_GESTURE_INVALID;
    }
    m_lastError = MIP_ERROR_NONE;
    return gestureEvent;
}

// This internal protected method sends the set gesture/radar mode command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetGestureRadarMode(MiPGestureRadarMode mode)
{
    uint8_t command[1+1] = { MIP_CMD_SET_GESTURE_RADAR_MODE, mode };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get gesture/radar mode command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetGestureRadarMode(MiPGestureRadarMode& mode)
{
    const uint8_t getGestureRadarMode[1] = { MIP_CMD_GET_GESTURE_RADAR_MODE };
    uint8_t       response[1+1];
    size_t        responseLength;
    int8_t        result = rawReceive(getGestureRadarMode, sizeof(getGestureRadarMode), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != 2 ||
        response[0] != MIP_CMD_GET_GESTURE_RADAR_MODE ||
        (response[1] != MIP_GESTURE_RADAR_DISABLED &&
         response[1] != MIP_GESTURE &&
         response[1] != MIP_RADAR))
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    mode = (MiPGestureRadarMode)response[1];
    return MIP_ERROR_NONE;
}

float MiP::readDistanceTravelled()
{
    int8_t result;
	
    // Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        float distance;
        result = rawReadOdometer(distance);
        if (result == MIP_ERROR_NONE)
        {
            m_lastError = MIP_ERROR_NONE;
            return distance;
        }
		
        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
    return 0.0f;
}

void MiP::resetDistanceTravelled()
{
    uint8_t command[1] = { MIP_CMD_RESET_ODOMETER };
	
    // Send this command blindly with no error checking since there is no robust way to determine if it has failed.
    rawSend(command, sizeof(command));
}

// This internal protected method sends the read odometer command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawReadOdometer(float& distanceInCm)
{
    const uint8_t readOdometer[1] = { MIP_CMD_READ_ODOMETER };
    uint8_t       response[1+4];
    size_t        responseLength;
    int           result = rawReceive(readOdometer, sizeof(readOdometer), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_READ_ODOMETER)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
	
	// Tick count is stored as big-endian in response buffer.
    uint32_t ticks = (uint32_t)response[1] << 24 | (uint32_t)response[2] << 16 | (uint32_t)response[3] << 8 | response[4];
	
    // Odometer has 48.5 ticks / cm.
    distanceInCm = (float)((double)ticks / 48.5);
    return result;
}

float MiP::readBatteryVoltage()
{

    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
	
    m_lastError = MIP_ERROR_NONE;
    return m_lastStatus.battery;
}

MiPPosition MiP::readPosition()
{

    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
	
    m_lastError = MIP_ERROR_NONE;
    return m_lastStatus.position;
}

bool MiP::isOnBack() { return readPosition() == MIP_POSITION_ON_BACK; }
bool MiP::isFaceDown() { return readPosition() == MIP_POSITION_FACE_DOWN; }
bool MiP::isUpright() { return readPosition() == MIP_POSITION_UPRIGHT; }
bool MiP::isPickedUp() { return readPosition() == MIP_POSITION_PICKED_UP; }
bool MiP::isHandStanding() { return readPosition() == MIP_POSITION_HAND_STAND; }
bool MiP::isFaceDownOnTray() { return readPosition() == MIP_POSITION_FACE_DOWN_ON_TRAY; }
bool MiP::isOnBackWithKickstand() { return readPosition() == MIP_POSITION_ON_BACK_WITH_KICKSTAND; }

// This internal protected method sends the get status command with minimal error handling. The error
// recovery happens at a higher level of the driver in begin(). All status updates after begin() come from events.
int8_t MiP::rawGetStatus(MiPStatus& status)
{
    const uint8_t getStatus[1] = { MIP_CMD_GET_STATUS };
    uint8_t       response[1+2];
    size_t        responseLength;
    int           result = rawReceive(getStatus, sizeof(getStatus), response, sizeof(response), responseLength);
    if (result) return result;
    return parseStatus(status, response, responseLength);
}

// This internal protected method takes the status response, validates it, converts it into convenient units and
// packs the result into a MiPStatus class.
int8_t MiP::parseStatus(MiPStatus& status, const uint8_t response[], size_t responseLength)
{
    if (responseLength != 3 ||
        response[0] != MIP_CMD_GET_STATUS ||
        response[2] > MIP_POSITION_ON_BACK_WITH_KICKSTAND)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }

    // Convert battery integer value to floating point voltage value.
    status.battery = (float)(((response[1] - 0x4D) / (float)(0x7C - 0x4D)) * (6.4f - 4.0f)) + 4.0f;
    status.position = (MiPPosition)response[2];
    return MIP_ERROR_NONE;
}

int8_t MiP::readWeight()
{

    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    if ((m_flags & MIP_FLAG_WEIGHT_VALID))
    {
	    // Have a cached weight event already, so just return it.
        m_lastError = MIP_ERROR_NONE;
        return m_lastWeight;
    }

    // Haven't seen a weight event yet so request the weight explicitly.
    // Retry the read if it should fail on the first attempt.
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        int8_t weight;
        result = rawGetWeight(weight);
        if (result == MIP_ERROR_NONE)
        {
            // Cache the returned value and return it to the caller.
            m_lastError = MIP_ERROR_NONE;
            m_lastWeight = weight;
            m_flags |= MIP_FLAG_WEIGHT_VALID;
            return weight;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
    return 0;
}

// This internal protected method sends the get weight command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetWeight(int8_t& weight)
{
    const uint8_t getWeight[1] = { MIP_CMD_GET_WEIGHT };
    uint8_t       response[1+1];
    size_t        responseLength;
    int8_t        result = rawReceive(getWeight, sizeof(getWeight), response, sizeof(response), responseLength);
    if (result) return result;
    return parseWeight(weight, response, responseLength);
}

// This internal protected method takes the weight response and validates it.
int8_t MiP::parseWeight(int8_t& weight, const uint8_t response[], size_t responseLength)
{
    if (responseLength != 2 || response[0] != MIP_CMD_GET_WEIGHT)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    weight = response[1];
    return MIP_ERROR_NONE;
}

void MiP::enableClapEvents() { checkedEnableClapEvents(MIP_CLAP_ENABLED); }
void MiP::disableClapEvents() { checkedEnableClapEvents(MIP_CLAP_DISABLED); }

// This internal protected method attempts to enable/disable clap events and then reads back the clap settings to see
// if the new value has taken. Retries on errors or mismatches.
void MiP::checkedEnableClapEvents(MiPClapEnabled enabled)
{
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawEnableClap(enabled);

        // Read back and make sure that it was set as expected.
        MiPClapSettings setting;
        result = rawGetClapSettings(setting);
        if (result == MIP_ERROR_NONE && setting.enabled == enabled)
        {
			// The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
	
    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from read back routine.
        m_lastError = result;
    }
    else
    {
        // Read back was successful but write didn't take.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

void MiP::writeClapDelay(uint16_t delayTime)
{
    int8_t result;

    // Send the set command and then issue the corresponding get command. Retry if the get fails or doesn't return the
    // expected new setting.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetClapDelay(delayTime);

        // Read back and make sure that it was set as expected.
        MiPClapSettings setting;
        result = rawGetClapSettings(setting);
        if (result == MIP_ERROR_NONE && setting.delay == delayTime)
        {
            // The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
	
    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from read back routine.
        m_lastError = result;
    }
    else
    {
        // Read back was successful but write didn't take.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

// This internal protected method sends the enable/disable clap command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawEnableClap(MiPClapEnabled enabled)
{
    uint8_t command[1+1] = { MIP_CMD_ENABLE_CLAP, enabled };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the set clap delay command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetClapDelay(uint16_t delay)
{
    uint8_t command[1+2] = { MIP_CMD_SET_CLAP_DELAY, (uint8_t)(delay >> 8), (uint8_t)(delay & 0xFF) };
    rawSend(command, sizeof(command));
}

bool MiP::areClapEventsEnabled()
{
    MiPClapSettings settings;
    int8_t result = readClapSettings(settings);
    if (result != MIP_ERROR_NONE)
    {
        m_lastError = result;
        return false;
    }
    m_lastError = MIP_ERROR_NONE;
    return settings.enabled == MIP_CLAP_ENABLED;
}

uint16_t MiP::readClapDelay()
{
    MiPClapSettings settings;
    int8_t result = readClapSettings(settings);
    if (result != MIP_ERROR_NONE)
    {
        m_lastError = result;
        return 0;
    }
    m_lastError = MIP_ERROR_NONE;
    return settings.delay;
}

// This internal protected method issues the low level get clap settings command and retries if an error is encountered.
int8_t MiP::readClapSettings(MiPClapSettings& settings)
{
    int8_t result;

    // Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        result = rawGetClapSettings(settings);
        if (result == MIP_ERROR_NONE) return MIP_ERROR_NONE;

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    settings.clear();
    return result;
}

uint8_t MiP::availableClapEvents()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
	
    m_lastError = MIP_ERROR_NONE;
    return m_clapEvents.available();
}

uint8_t MiP::readClapEvent()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
	
    uint8_t clapEvent = 0;
    if (!m_clapEvents.pop(clapEvent))
    {
		// No clap event has been received yet.
        m_lastError = MIP_ERROR_NO_EVENT;
        return 0;
    }
    m_lastError = MIP_ERROR_NONE;
    return clapEvent;
}

// This internal protected method sends the get clap settings command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetClapSettings(MiPClapSettings& settings)
{
    const uint8_t getClapSettings[1] = { MIP_CMD_GET_CLAP_SETTINGS };
    uint8_t       response[1+3];
    size_t        responseLength;
    int8_t        result = rawReceive(getClapSettings, sizeof(getClapSettings), response, sizeof(response), responseLength);
	
    if (result) return result;
	
    if (responseLength != sizeof(response) ||
        response[0] != MIP_CMD_GET_CLAP_SETTINGS ||
        (response[1] != MIP_CLAP_DISABLED && response[1] != MIP_CLAP_ENABLED))
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
	
    settings.enabled = (MiPClapEnabled)response[1];
    settings.delay = (uint16_t)response[2] << 8 | response[3];
    return MIP_ERROR_NONE;
}

bool MiP::hasBeenShaken()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    m_lastError = MIP_ERROR_NONE;
    if (m_flags & MIP_FLAG_SHAKE_DETECTED)
    {
        // A shake event has been received since the last call to this function. Return true and clear the shake
        // detected bit.
        m_flags &= ~MIP_FLAG_SHAKE_DETECTED;
        return true;
    }
    return false;
}

void MiP::readSoftwareVersion(MiPSoftwareVersion& software)
{
    int8_t result;

    // Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        result = rawGetSoftwareVersion(software);
        if (result == MIP_ERROR_NONE)
        {
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
}

void MiP::readHardwareInfo(MiPHardwareInfo& hardware)
{
    int8_t result;

    // Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        result = rawGetHardwareInfo(hardware);
        if (result == MIP_ERROR_NONE)
        {
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
}

// This internal protected method sends the get software version command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetSoftwareVersion(MiPSoftwareVersion& software)
{
    const uint8_t getSoftwareVersion[1] = { MIP_CMD_GET_SOFTWARE_VERSION };
    uint8_t       response[1+4];
    size_t        responseLength;
    int8_t        result = rawReceive(getSoftwareVersion, sizeof(getSoftwareVersion), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_GET_SOFTWARE_VERSION)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    software.year = 2000 + response[1];
    software.month = response[2];
    software.day = response[3];
    software.uniqueVersion = response[4];
    return result;
}

// This internal protected method sends the get hardware info command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetHardwareInfo(MiPHardwareInfo& hardware)
{
    const uint8_t getHardwareInfo[1] = { MIP_CMD_GET_HARDWARE_INFO };
    uint8_t       response[1+2];
    size_t        responseLength;
    int8_t        result = rawReceive(getHardwareInfo, sizeof(getHardwareInfo), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_GET_HARDWARE_INFO)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    hardware.voiceChip = response[1];
    hardware.hardware = response[2];
    return result;
}

void MiP::enableAppMode() { verifiedSetGameMode(MIP_APP_MODE); }
void MiP::enableCageMode() { verifiedSetGameMode(MIP_CAGE_MODE); }
void MiP::enableDanceMode() { verifiedSetGameMode(MIP_DANCE_MODE); }
void MiP::enableStackMode() { verifiedSetGameMode(MIP_STACK_MODE); }
void MiP::enableTrickMode() { verifiedSetGameMode(MIP_TRICK_MODE); }
void MiP::enableRoamMode() { verifiedSetGameMode(MIP_ROAM_MODE); }

bool MiP::isAppModeEnabled() { return checkGameMode(MIP_APP_MODE); }
bool MiP::isCageModeEnabled() { return checkGameMode(MIP_CAGE_MODE); }
bool MiP::isDanceModeEnabled() { return checkGameMode(MIP_DANCE_MODE); }
bool MiP::isStackModeEnabled() { return checkGameMode(MIP_STACK_MODE); }
bool MiP::isTrickModeEnabled() { return checkGameMode(MIP_TRICK_MODE); }
bool MiP::isRoamModeEnabled() { return checkGameMode(MIP_ROAM_MODE); }

bool MiP::checkGameMode(MiPGameMode expectedMode)
{
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        MiPGameMode currentMode;
        result = rawGetGameMode(currentMode);
        if (result == MIP_ERROR_NONE) return currentMode == expectedMode;

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
    return false;
}

// This internal protected method sends the command to change the game mode and then sends a request to get
// the new mode. If this request fails or the new mode isn't as expected, it will retry the command.
void MiP::verifiedSetGameMode(MiPGameMode desiredMode)
{
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetGameMode(desiredMode);

        // Read back and make sure that it was set as expected.
        MiPGameMode actualMode;
        result = rawGetGameMode(actualMode);
        if (result == MIP_ERROR_NONE && actualMode == desiredMode)
        {
            // The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }
		
        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }

    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from rawGetGameMode().
        m_lastError = result;
    }
    else
    {
        // rawGetGameMode() was successful but didn't match mode to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

// This internal protected method sends the set game mode command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetGameMode(MiPGameMode mode)
{
    // Might not accept command if currently running another game mode so Stop first.
    stop();
	
    uint8_t command[1+1] = { MIP_CMD_SET_GAME_MODE, mode };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get game mode command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetGameMode(MiPGameMode& mode)
{
    const uint8_t getGameMode[1] = { MIP_CMD_GET_GAME_MODE };
    uint8_t       response[1+1];
    size_t        responseLength;
	
    // Might not accept get game mode command when currently running a game mode so Stop first.
    stop();
	
    int8_t result = rawReceive(getGameMode, sizeof(getGameMode), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != 2 ||
        response[0] != MIP_CMD_GET_GAME_MODE ||
        (response[1] != MIP_APP_MODE &&
         response[1] != MIP_CAGE_MODE &&
         response[1] != MIP_TRACKING_MODE &&
         response[1] != MIP_DANCE_MODE &&
         response[1] != MIP_DEFAULT_MODE &&
         response[1] != MIP_STACK_MODE &&
         response[1] != MIP_TRICK_MODE &&
         response[1] != MIP_ROAM_MODE))
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    mode = (MiPGameMode)response[1];

    // Restart the game mode now that we have successfully retrieved it.
    rawSetGameMode(mode);
	
    return MIP_ERROR_NONE;
}

void MiP::setUserData(uint8_t addressOffset, uint8_t userData)
{
    uint8_t address = BASE_EEPROM_ADDRESS + addressOffset;
	
	// Address must be between 0x20 and 0x2F, inclusive.
    MIP_ASSERT( BASE_EEPROM_ADDRESS <= address && address <= LAST_EEPROM_ADDRESS );
	
    int8_t result;
	
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetUserData(address, userData);

        // Read back and make sure that it was set as expected.
        byte storedData = 0x00;
        result = rawGetUserData(address, storedData);
        if (result == MIP_ERROR_NONE && storedData == userData)
        {
            // The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }

    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from rawGetUserData().
        m_lastError = result;
    }
    else
    {
        // rawGetUserData() was successful but didn't match the data we were expecting.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

uint8_t MiP::getUserData(uint8_t addressOffset)
{
    uint8_t address = BASE_EEPROM_ADDRESS + addressOffset;

    // Address must be between 0x20 and 0x2F, inclusive.
    MIP_ASSERT( BASE_EEPROM_ADDRESS <= address && address <= LAST_EEPROM_ADDRESS );
	
    int8_t result;

    // Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        uint8_t storedData;
        result = rawGetUserData(address, storedData);
        if (result == MIP_ERROR_NONE)
        {
            m_lastError = MIP_ERROR_NONE;
            return storedData;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }
    m_lastError = result;
    return 0;
}

// This internal protected method sends the set user data command with no error checking.
// The error handling and recovery happens at a higher level of the driver.
void MiP::rawSetUserData(uint8_t address, uint8_t userData)
{
    uint8_t command[1+2] = { MIP_CMD_SET_USER_DATA, address, userData };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get user data command with minimal error handling.
// The error and recovery happens at a higher level of the driver.
int8_t MiP::rawGetUserData(uint8_t address, uint8_t& userData)
{
    uint8_t getUserData[1+1] = { MIP_CMD_GET_USER_DATA, address };
    uint8_t       response[1+2];
    size_t        responseLength;
    int8_t        result = rawReceive(getUserData, sizeof(getUserData), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != 3 ||
        response[0] != MIP_CMD_GET_USER_DATA ||
        response[1] != address)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    userData = (uint8_t)response[2];
    return MIP_ERROR_NONE;
}

void MiP::enableMiPDetectionMode(uint8_t id, uint8_t txPower)
{
    m_irId = id;
    rawSetMiPDetectionMode(id, txPower);
}

void MiP::disableMiPDetectionMode()
{
    m_irId = MIP_IR_DETECTION_MODE_DISABLE;

    // According to WowWee documentation, TX power must be between 1 and 120 even when disabling.
    rawSetMiPDetectionMode(MIP_IR_DETECTION_MODE_DISABLE, 0x01);
}

bool MiP::isMiPDetectionModeEnabled() { return m_irId > MIP_IR_DETECTION_MODE_DISABLE; }

uint8_t MiP::readDetectedMiP()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    uint8_t detectedMiPEvent = 0;
    if(!m_detectedMiPEvents.pop(detectedMiPEvent))
    {
        m_lastError = MIP_ERROR_NO_EVENT;
        return detectedMiPEvent;
    }
    m_lastError = MIP_ERROR_NONE;
    return detectedMiPEvent;
}

uint8_t MiP::availableDetectedMiPEvents()
{
	// Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    m_lastError = MIP_ERROR_NONE;
    return m_detectedMiPEvents.available();
}

// This internal protected method sends the set detection mode command with minimal error
// handling. The error recovery happens at a higher level of the driver.
void MiP::rawSetMiPDetectionMode(uint8_t id, uint8_t txPower)
{
    MIP_ASSERT( 0x01 <= txPower && txPower <= 0x78 );
    uint8_t command[1+2] = { MIP_CMD_SET_DETECTION_MODE, id, txPower };
    rawSend(command, sizeof(command));
}

void MiP::enableIRRemoteControl() { verifiedIRRemoteControl(MIP_IR_REMOTE_CONTROL_ENABLE); }
void MiP::disableIRRemoteControl() { verifiedIRRemoteControl(MIP_IR_REMOTE_CONTROL_DISABLE); }

bool MiP::isIRRemoteControlEnabled()
{
    const uint8_t remoteControlEnabled[1] = { MIP_CMD_GET_IR_REMOTE_CONTROL };
    uint8_t       response[1+1];
    size_t        responseLength;
    int8_t        result = rawReceive(remoteControlEnabled, sizeof(remoteControlEnabled), response, sizeof(response), responseLength);
    if (result != MIP_ERROR_NONE)
    {
        m_lastError = result;
        return false;
    }
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_GET_IR_REMOTE_CONTROL)
    {
        m_lastError = MIP_ERROR_BAD_RESPONSE;
        return false;
    }
    m_lastError = MIP_ERROR_NONE;
    return response[1] == MIP_IR_REMOTE_CONTROL_ENABLE;
}

void MiP::sendIRDongleCode(uint16_t sendCode, uint8_t transmitPower)
{
    uint8_t command[1+6] = {
        MIP_CMD_SEND_IR_DONGLE_CODE,
        0x00, 0x00,
        (uint8_t)((sendCode >> 8) & 0xFF),
        (uint8_t)(sendCode & 0xFF),
        0x10, transmitPower
    };

    // Send this command blindly with no error checking since there is no robust way to determine if it has failed.
    rawSend(command, sizeof(command));
}

uint32_t MiP::readIRDongleCode()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    uint32_t irCodeEvent = 0xFFFFFFFF;
    if (!m_irCodeEvents.pop(irCodeEvent))
    {
        m_lastError = MIP_ERROR_NO_EVENT;
        return irCodeEvent;
    }
    m_lastError = MIP_ERROR_NONE;
    return irCodeEvent;
}

uint8_t MiP::availableIRCodeEvents()
{
    // Fetch bytes from the Serial receive buffer and process any event data found within.
    processAllResponseData();
    m_lastError = MIP_ERROR_NONE;
    return m_irCodeEvents.available();
}

// This internal protected method verifies that IR remote control is enabled.
void MiP::verifiedIRRemoteControl(uint8_t desiredRemoteControlMode)
{
    int8_t result;
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetIRRemoteControl(desiredRemoteControlMode);
        uint8_t actualMode;

        // Read back and make sure that it was set as expected.
        result = rawGetIRRemoteControl(actualMode);
        if (result == MIP_ERROR_NONE && actualMode == desiredRemoteControlMode)
        {
            // The set was successful so return immediately.
            m_lastError = MIP_ERROR_NONE;
            return;
        }

        // An error was encountered so we will loop around and try again.
        // Wait for a bit before the next retry.
        delay(MIP_RETRY_WAIT);
    }

    if (result != MIP_ERROR_NONE)
    {
        // Kept getting an error back from rawGetIRRemoteControl().
        m_lastError = result;
    }
    else
    {
        // rawGetGameMode() was successful but didn't match mode to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

// This internal protected method sends the set IR remote control command with minimal error
// handling. The error recovery happens at a higher level of the driver.
void MiP::rawSetIRRemoteControl(uint8_t remoteControl)
{
    MIP_ASSERT( remoteControl == MIP_IR_REMOTE_CONTROL_ENABLE ||  remoteControl == MIP_IR_REMOTE_CONTROL_DISABLE);
    uint8_t command[1+1] = { MIP_CMD_SET_IR_REMOTE_CONTROL, remoteControl };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get IR remote control status command with minimal
// error handling. The error recovery happens at a higher level of the driver.
int8_t MiP::rawGetIRRemoteControl(uint8_t& remoteControl)
{
    const uint8_t getIRRemoteControl[1] = { MIP_CMD_GET_IR_REMOTE_CONTROL };
    uint8_t       response[1+1];
    size_t        responseLength;
    int8_t        result = rawReceive(getIRRemoteControl, sizeof(getIRRemoteControl), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_GET_IR_REMOTE_CONTROL)
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    remoteControl = response[1];
    return result;
}

