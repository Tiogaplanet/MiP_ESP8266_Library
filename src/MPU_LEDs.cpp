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
/* Implements RGB flashing commands, solid color updates for the chest plate LED, and status
   configurations of the head LEDs.
*/
#include "MPU_D1_mini.h"

#define MIP_MAX_RETRIES 2
#define MIP_RETRY_WAIT 50

// MiP Protocol Commands related to the head and chest LEDs.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_GET_CHEST_LED 0x83
#define MIP_CMD_SET_CHEST_LED 0x84
#define MIP_CMD_FLASH_CHEST_LED 0x89
#define MIP_CMD_SET_HEAD_LEDS 0x8A
#define MIP_CMD_GET_HEAD_LEDS 0x8B

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber)
{
    MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_LEDs.cpp: %d\n", lineNumber);
    while (1) { delay(100); }
}

void MiP::writeChestLED(uint8_t red, uint8_t green, uint8_t blue)
{
    int8_t result;
	
	// The blue channel is actually only 6-bit and not a full 8-bit so zero out lower 2 bits (the MiP does this too).
    blue &= ~3;

    // Send the set command and then issue the corresponding get command. Retry if the get fails or doesn't return the
    // expected new setting.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetChestLED(red, green, blue);
		
		// Read back and make sure that it was set as expected.
        MiPChestLED actualChestLED;
        result = rawGetChestLED(actualChestLED);
        if (result == MIP_ERROR_NONE &&
            actualChestLED.red == red &&
            actualChestLED.green == green &&
            actualChestLED.blue == blue)
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
        // Kept getting an error back from read attempt.
        m_lastError = result;
    }
    else
    {
        // Read was successful but didn't match setting to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

void MiP::writeChestLED(uint8_t red, uint8_t green, uint8_t blue, uint16_t onTime, uint16_t offTime)
{
    int8_t result;
	
	// on/off time are in units of 20 msecs.
    MIP_ASSERT( onTime / 20 <= 255 && offTime / 20 <= 255 );
    onTime = (onTime + 10) / 20;
    offTime = (offTime + 10) / 20;
	
	// The blue channel is actually only 6-bit and not a full 8-bit so zero out lower 2 bits (the MiP does this too).
    blue &= ~3;

    // Send the set command and then issue the corresponding get command. Retry if the get fails or doesn't return the
    // expected new setting.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawFlashChestLED(red, green, blue, onTime, offTime);
		
		// Read back and make sure that it was set as expected.
        MiPChestLED actualChestLED;
        result = rawGetChestLED(actualChestLED);
        if (result == MIP_ERROR_NONE &&
            actualChestLED.red == red &&
            actualChestLED.green == green &&
            actualChestLED.blue == blue &&
            actualChestLED.onTime == onTime &&
            actualChestLED.offTime == offTime)
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
        // Kept getting an error back from read attempt.
        m_lastError = result;
    }
    else
    {
        // Read was successful but didn't match setting to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

void MiP::writeChestLED(const MiPChestLED& chestLED)
{
    writeChestLED(chestLED.red, chestLED.green, chestLED.blue, chestLED.onTime, chestLED.offTime);
}

void MiP::readChestLED(MiPChestLED& chestLED)
{
    int8_t result;
	
	// Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        result = rawGetChestLED(chestLED);
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

void MiP::unverifiedWriteChestLED(uint8_t red, uint8_t green, uint8_t blue)
{
    rawSetChestLED(red, green, blue);
}

void MiP::unverifiedWriteChestLED(uint8_t red, uint8_t green, uint8_t blue, uint16_t onTime, uint16_t offTime)
{
	// on/off time are in units of 20 msecs.
    MIP_ASSERT( onTime / 20 <= 255 && offTime / 20 <= 255 );
    onTime = (onTime + 10) / 20;
    offTime = (offTime + 10) / 20;
    rawFlashChestLED(red, green, blue, onTime, offTime);
}

void MiP::unverifiedWriteChestLED(const MiPChestLED& chestLED)
{
    unverifiedWriteChestLED(chestLED.red, chestLED.green, chestLED.blue, chestLED.onTime, chestLED.offTime);
}

// This internal protected method sends the set chest LED command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetChestLED(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t command[1+3] = { MIP_CMD_SET_CHEST_LED, red, green, blue };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the flash chest LED command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawFlashChestLED(uint8_t red, uint8_t green, uint8_t blue, uint8_t onTime, uint8_t offTime)
{
    uint8_t command[1+5] = { MIP_CMD_FLASH_CHEST_LED, red, green, blue, onTime, offTime };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get chest LED command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetChestLED(MiPChestLED& chestLED)
{
    const uint8_t getChestLED[1] = { MIP_CMD_GET_CHEST_LED };
    uint8_t       response[1+5];
    size_t        responseLength;
    uint8_t       result = rawReceive(getChestLED, sizeof(getChestLED), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) || response[0] != MIP_CMD_GET_CHEST_LED )
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    chestLED.red = response[1];
    chestLED.green = response[2];
    chestLED.blue = response[3];
	
	// on/off time are in units of 20 msecs.
    chestLED.onTime = (uint16_t)response[4] * 20;
    chestLED.offTime = (uint16_t)response[5] * 20;
    return MIP_ERROR_NONE;
}

void MiP::writeHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4)
{
    int8_t result;
	
	// Send the set command and then issue the corresponding get command. Retry if the get fails or doesn't return the
    // expected new setting.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        rawSetHeadLEDs(led1, led2, led3, led4);
		
		// Read back and make sure that it was set as expected.
        MiPHeadLEDs headLEDs;
        result = rawGetHeadLEDs(headLEDs);
        if (result == MIP_ERROR_NONE &&
            headLEDs.led1 == led1 &&
            headLEDs.led2 == led2 &&
            headLEDs.led3 == led3 &&
            headLEDs.led4 == led4)
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
        // Kept getting an error back from read attempt.
        m_lastError = result;
    }
    else
    {
        // Read was successful but didn't match setting to which we were attempting to change.
        m_lastError = MIP_ERROR_MAX_RETRIES;
    }
}

void MiP::writeHeadLEDs(const MiPHeadLEDs& headLEDs)
{
    writeHeadLEDs(headLEDs.led1, headLEDs.led2, headLEDs.led3, headLEDs.led4);
}

void MiP::readHeadLEDs(MiPHeadLEDs& headLEDs)
{
    int8_t result;
	
	// Retry the read if it should fail on the first attempt.
    for (uint8_t retry = 0 ; retry < MIP_MAX_RETRIES ; retry++)
    {
        result = rawGetHeadLEDs(headLEDs);
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

void MiP::unverifiedWriteHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4)
{
    rawSetHeadLEDs(led1, led2, led3, led4);
}

void MiP::unverifiedWriteHeadLEDs(const MiPHeadLEDs& headLEDs)
{
    unverifiedWriteHeadLEDs(headLEDs.led1, headLEDs.led2, headLEDs.led3, headLEDs.led4);
}

// This internal protected method sends the set head LEDs command with no error checking. The error handling /
// recovery happens at a higher level of the driver.
void MiP::rawSetHeadLEDs(MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4)
{
    uint8_t command[1+4] = { MIP_CMD_SET_HEAD_LEDS, led1, led2, led3, led4 };
    rawSend(command, sizeof(command));
}

// This internal protected method sends the get head LEDs command with minimal error handling. The error
// recovery happens at a higher level of the driver.
int8_t MiP::rawGetHeadLEDs(MiPHeadLEDs& headLEDs)
{
    const uint8_t getHeadLEDs[1] = { MIP_CMD_GET_HEAD_LEDS };
    uint8_t       response[1+4];
    size_t        responseLength;
    int           result = rawReceive(getHeadLEDs, sizeof(getHeadLEDs), response, sizeof(response), responseLength);
    if (result) return result;
    if (responseLength != sizeof(response) ||
        response[0] != (uint8_t)MIP_CMD_GET_HEAD_LEDS ||
        !isValidHeadLED(response[1]) ||
        !isValidHeadLED(response[2]) ||
        !isValidHeadLED(response[3]) ||
        !isValidHeadLED(response[4]))
    {
        return MIP_ERROR_BAD_RESPONSE;
    }
    headLEDs.led1 = (MiPHeadLED)response[1];
    headLEDs.led2 = (MiPHeadLED)response[2];
    headLEDs.led3 = (MiPHeadLED)response[3];
    headLEDs.led4 = (MiPHeadLED)response[4];
    return MIP_ERROR_NONE;
}

// This internal protected method is called to validate that each head LED value returned is within the expected
// range.
bool MiP::isValidHeadLED(uint8_t led)
{
    return led <= MIP_HEAD_LED_BLINK_FAST;
}
