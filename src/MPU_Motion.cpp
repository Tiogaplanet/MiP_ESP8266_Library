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
/* Holds the drive, turn, stop, and positional capabilities.
*/
#include "MPU_D1_mini.h"

#define MIP_CONTINUOUS_DRIVE_DELAY 50

// MiP Protocol Commands related to motion.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_CONTINUOUS_DRIVE 0x78
#define MIP_CMD_DISTANCE_DRIVE   0x70
#define MIP_CMD_TURN_LEFT        0x73
#define MIP_CMD_TURN_RIGHT       0x74
#define MIP_CMD_DRIVE_FORWARD    0x71
#define MIP_CMD_DRIVE_BACKWARD   0x72
#define MIP_CMD_STOP             0x77
#define MIP_CMD_SET_POSITION     0x08
#define MIP_CMD_GET_UP           0x23

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber)
{
    MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_Motion.cpp: %d\n", lineNumber);
    while (1) { delay(100); }
}

void MiP::continuousDrive(int8_t velocity, int8_t turnRate)
{
    uint8_t command[1+2];

    MIP_ASSERT( velocity >= -32 && velocity <= 32 );
    MIP_ASSERT( turnRate >= -32 && turnRate <= 32 );

    // Ignore requests if they come in too fast so that it can be done in a tight loop but not overload MiP.
    if (millis() - m_lastContinuousDriveTime < MIP_CONTINUOUS_DRIVE_DELAY)
    {
        m_lastError = MIP_ERROR_NONE;
        return;
    }
    m_lastContinuousDriveTime = millis();

    command[0] = MIP_CMD_CONTINUOUS_DRIVE;
    command[1] = (velocity == 0) ? 0x00 : ((velocity < 0) ? (0x20 + (-velocity)) : velocity);
    command[2] = (turnRate == 0) ? 0x00 : ((turnRate < 0) ? (0x60 + (-turnRate)) : (0x40 + turnRate));

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::distanceDrive(MiPDriveDirection driveDirection, uint8_t cm, MiPTurnDirection turnDirection, uint16_t degrees)
{
    uint8_t command[1+5];
    MIP_ASSERT( degrees <= 360 );

    command[0] = MIP_CMD_DISTANCE_DRIVE;
    command[1] = driveDirection;
    command[2] = cm;
    command[3] = turnDirection;
    command[4] = degrees >> 8;
    command[5] = degrees & 0xFF;

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::turnLeft(uint16_t degrees, uint8_t speed)
{
	// The turn command is in units of 5 degrees.
    uint8_t angle = degrees / 5;
    uint8_t command[1+2];

    MIP_ASSERT( degrees <= 255 * 5 );
    MIP_ASSERT( speed <= 24 );

    command[0] = MIP_CMD_TURN_LEFT;
    command[1] = angle;
    command[2] = speed;

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::turnRight(uint16_t degrees, uint8_t speed)
{
	// The turn command is in units of 5 degrees.
    uint8_t angle = degrees / 5;
    uint8_t command[1+2];

    MIP_ASSERT( degrees <= 255 * 5 );
    MIP_ASSERT( speed <= 24 );

    command[0] = MIP_CMD_TURN_RIGHT;
    command[1] = angle;
    command[2] = speed;

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::driveForward(uint8_t speed, uint16_t time)
{
	// The time parameters is in units of 7 milliseconds.
    uint8_t command[1+2];

    MIP_ASSERT( speed <= 30 );
    MIP_ASSERT( time <= 255 * 7 );

    command[0] = MIP_CMD_DRIVE_FORWARD;
    command[1] = speed;
    command[2] = time / 7;

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::driveBackward(uint8_t speed, uint16_t time)
{
	// The time parameters is in units of 7 milliseconds.
    uint8_t command[1+2];

    MIP_ASSERT( speed <= 30 );
    MIP_ASSERT( time <= 255 * 7 );

    command[0] = MIP_CMD_DRIVE_BACKWARD;
    command[1] = speed;
    command[2] = time / 7;

    // Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::stop()
{
    uint8_t command[1] = { MIP_CMD_STOP };
	
	// Send this command blindly with no error checking since there is no way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}

void MiP::fallForward()
{
    fallDown(MIP_FALL_FACE_DOWN);
    m_lastError = MIP_ERROR_NONE;
}

void MiP::fallBackward()
{
    fallDown(MIP_FALL_ON_BACK);
    m_lastError = MIP_ERROR_NONE;
}

// This internal protected method sends the desired set position command to fall forward or backward.
void MiP::fallDown(MiPFallDirection direction)
{
    uint8_t command[1+1];
    command[0] = MIP_CMD_SET_POSITION;
    command[1] = direction;
	
	// Send this command blindly with no error checking since there is no easy way to determine if it has failed.
    rawSend(command, sizeof(command));
}

void MiP::getUp(MiPGetUp getup /* = MIP_GETUP_FROM_EITHER */)
{
    uint8_t command[1+1];
    command[0] = MIP_CMD_GET_UP;
    command[1] = getup;

    // Send this command blindly with no error checking since there is no easy way to determine if it has failed.
    rawSend(command, sizeof(command));
    m_lastError = MIP_ERROR_NONE;
}
