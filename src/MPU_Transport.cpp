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
/* Handles the low-level byte transmission, hex-to-binary parsing, and the critical Out-of-Band
   (OOB) response reading.
*/
#include "MPU_D1_mini.h"

#define MIP_REQUEST_DELAY 8
#define MIP_RESPONSE_TIMEOUT 100

// expectResponse parameter values for transportSendRequest() parameter.
#define MIP_EXPECT_NO_RESPONSE 0
#define MIP_EXPECT_RESPONSE    1

// MiP Protocol Commands related to sensors.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for the complete list.
#define MIP_CMD_RECEIVE_IR_DONGLE_CODE  0x03
#define MIP_CMD_GET_DETECTED_MIP        0x04
#define MIP_CMD_GET_GESTURE_RESPONSE    0x0A
#define MIP_CMD_GET_RADAR_RESPONSE      0x0C
#define MIP_CMD_GET_VOLUME              0x16
#define MIP_CMD_SHAKE_RESPONSE          0x1A
#define MIP_CMD_CLAP_RESPONSE           0x1D
#define MIP_CMD_GET_STATUS              0x79
#define MIP_CMD_GET_WEIGHT              0x81

// Define an assert mechanism that can be used to log and halt when the user is found to be calling the API incorrectly.
#define MIP_ASSERT(EXPRESSION) if (!(EXPRESSION)) mipAssert(__LINE__);

static void mipAssert(uint32_t lineNumber)
{
    MIP_DEBUG_ERROR_PRINTF("MiP: Assert: MPU_Transport.cpp: %d\n", lineNumber);
    while (1) { delay(100); }
}

void MiP::rawSend(const uint8_t request[], size_t requestLength)
{
    transportSendRequest(request, requestLength, MIP_EXPECT_NO_RESPONSE);
}

int8_t MiP::rawReceive(const uint8_t request[], size_t requestLength,
                       uint8_t responseBuffer[], size_t responseBufferSize, size_t& responseLength)
{
    transportSendRequest(request, requestLength, MIP_EXPECT_RESPONSE);
    return transportGetResponse(responseBuffer, responseBufferSize, &responseLength);
}

void MiP::transportSendRequest(const uint8_t* pRequest, size_t requestLength, int expectResponse)
{
    // Must call begin() and have it return 'true' before calling sending commands to the MiP.
    MIP_ASSERT( isInitialized() );

    // Let the MiP process the last request before letting another request be issued.
    while (millis() - m_lastRequestTime < MIP_REQUEST_DELAY) { delay(1); }

    // Remember the command byte (first byte) if expecting a response to this request since the response should start
    // with the same byte.
    if (expectResponse) m_expectedResponseCommand = pRequest[0];
    else m_expectedResponseCommand = 0;

    m_expectedResponseSize = 0;
    m_responseBuffer[0] = 0;

    // Send the specified bytes to the MiP via the UART.
    while (requestLength-- > 0)
    {
        Serial.write(*pRequest++);
    }
    m_lastRequestTime = millis();
}

int8_t MiP::transportGetResponse(uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength)
{
    // Must call begin() and have it return 'true' before calling sending commands to the MiP.
    MIP_ASSERT( isInitialized() );
	
    // Caller is attempting to get a response that is larger than support by the MiP and this library.
    MIP_ASSERT( responseBufferSize <= MIP_RESPONSE_MAX_LEN);

    // UNDONE: I think it would be my bug if the following assert ever fired.
    MIP_ASSERT( m_expectedResponseCommand != 0 );

    // Process all received bytes (which might include out of band notifications) until we find the response to the
    // last request made. Will timeout after a second.
    m_expectedResponseSize = (uint8_t)responseBufferSize;
    uint32_t startTime = millis();
    bool responseFound = false;
    do
    {
        responseFound = processAllResponseData();
    } while (!responseFound && (uint32_t)millis() - startTime < MIP_RESPONSE_TIMEOUT);

    if (!responseFound)
    {
        // Never received the expected response within the timeout window.
        MIP_DEBUG_WARN_PRINTLN(F("MiP: Response timeout"));
        return MIP_ERROR_TIMEOUT;
    }

    // Copy reponse data into caller provided buffer and clear state in transport about the expected response.
    memcpy(pResponseBuffer, m_responseBuffer, m_expectedResponseSize);
    *pResponseLength = m_expectedResponseSize;
    m_expectedResponseCommand = 0;
    m_expectedResponseSize = 0;
    m_responseBuffer[0] = 0;

    return MIP_ERROR_NONE;
}

bool MiP::processAllResponseData()
{
    bool    responseFound = false;
    uint8_t buffer[(MIP_RESPONSE_MAX_LEN - 1) * 2];
    size_t  bytesToRead;
    size_t  bytesRead;

    while (Serial.available() >= 2)
    {
        uint8_t highNibble = Serial.read();
        uint8_t lowNibble = Serial.read();
        uint8_t commandByte = (parseHexDigit(highNibble) << 4) | parseHexDigit(lowNibble);

        if (m_expectedResponseCommand != 0 && commandByte == m_expectedResponseCommand)
        {
            // Store away the command byte that we just read into response buffer so that it isn't lost.
            m_responseBuffer[0] = commandByte;

            // Already read the command byte into element 0 of the response buffer earlier so just need to read in the
            // rest of the expected response bytes now.
            bytesToRead = m_expectedResponseSize - 1;
            bytesRead = Serial.readBytes(buffer, bytesToRead * 2);
            if (bytesRead == bytesToRead * 2)
            {
                copyHexTextToBinary(&m_responseBuffer[1], buffer, bytesToRead);
                responseFound = true;
                // Continue to process any other bytes in the recieve buffer.
                // This would allow something like a rawGetStatus() call to receive the actual data returned for this
                // request and not an older OOB perioidic status notification.
            }
            else
            {
                // Timed out waiting for all of the response data.
                m_expectedResponseCommand = 0;
                m_expectedResponseSize = 0;
                m_responseBuffer[0] = 0;
                MIP_DEBUG_ERROR_PRINTF("MiP: Response too short: %d, %d\n", bytesRead, bytesToRead * 2);
                break;
            }
        }
        else
        {
            processOobResponseData(commandByte);
        }
    }
    return responseFound;
}

void MiP::copyHexTextToBinary(uint8_t* pDest, uint8_t* pSrc, uint8_t length)
{
    while (length-- > 0)
    {
        *pDest = (parseHexDigit(pSrc[0]) << 4) | parseHexDigit(pSrc[1]);
        pDest++;
        pSrc += 2;
    }
}

uint8_t MiP::parseHexDigit(uint8_t digit)
{
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    return 0;
}

void MiP::processOobResponseData(uint8_t commandByte)
{
    size_t length = 0;
    size_t bytesRead;

    // The number of additional bytes to read depends on which notification has been found in serial buffer.
    switch (commandByte)
    {
    case MIP_CMD_GET_RADAR_RESPONSE:
    case MIP_CMD_GET_GESTURE_RESPONSE:
    case MIP_CMD_CLAP_RESPONSE:
    case MIP_CMD_GET_WEIGHT:
    case MIP_CMD_GET_DETECTED_MIP:
        length = 1;
        break;
    case MIP_CMD_SHAKE_RESPONSE:
        length = 0;
        break;
    case MIP_CMD_GET_STATUS:
        length = 2;
        break;
    case MIP_CMD_RECEIVE_IR_DONGLE_CODE:
        // MIP_CMD_RECEIVE_IR_DONGLE_CODE is the only message delivered by MiP that has a
        // variable length so we need to read the next byte which contains the length.
        uint8_t nibbles[2];
        bytesRead = Serial.readBytes(nibbles, sizeof(nibbles));
        if (bytesRead != sizeof(nibbles))
        {
            MIP_DEBUG_ERROR_PRINTLN(F("MiP: Missing IR code length"));
            return;
        }
        length = (parseHexDigit(nibbles[0]) << 4) | parseHexDigit(nibbles[1]);
        if (length < 2 || length > 4)
        {
            uint8_t discardedBytes = discardUnexpectedSerialData();
            MIP_DEBUG_ERROR_PRINTF("MiP: Bad IR code length: 0x%02x (discarded %d bytes)\n", length, discardedBytes);
            return;
        }
        break;
    default:
        uint8_t discardedBytes = discardUnexpectedSerialData();
        MIP_DEBUG_ERROR_PRINTF("MiP: Bad OOB command byte: 0x%02x (discarded %d bytes)\n", commandByte, discardedBytes);
        return;
    }

     // Read in the additional bytes of the notification.  The "4" comes from maximum length
    // which is a response for MIP_CMD_RECEIVE_IR_DONGLE_CODE.
	uint8_t buffer[4 * 2];
    bytesRead = Serial.readBytes(buffer, length * 2);
    if (bytesRead != length * 2)
    {
        MIP_DEBUG_ERROR_PRINTF("MiP: OOB too short: %d, %d", bytesRead, length * 2);
        return;
    }

    // Convert the hex data to a binary response.
    uint8_t response[MIP_RESPONSE_MAX_LEN];
    response[0] = commandByte;
    copyHexTextToBinary(&response[1], buffer, length);

    // Have 32 bits ready in case of an IR event.
    uint32_t irCode = 0;

    // Process the response just received.
    switch (commandByte)
    {
    case MIP_CMD_GET_RADAR_RESPONSE:
        if (response[1] >= MIP_RADAR_NONE && response[1] <= MIP_RADAR_0CM_10CM)
        {
            m_lastRadar = (MiPRadar)response[1];
            m_flags |= MIP_FLAG_RADAR_VALID;
        }
        break;
    case MIP_CMD_GET_GESTURE_RESPONSE:
        if (response[1] >= MIP_GESTURE_LEFT && response[1] <= MIP_GESTURE_BACKWARD)
        {
            m_gestureEvents.push((MiPGesture)response[1]);
        }
        break;
    case MIP_CMD_SHAKE_RESPONSE:
        m_flags |= MIP_FLAG_SHAKE_DETECTED;
        break;
    case MIP_CMD_GET_STATUS:
        parseStatus(m_lastStatus, response, length + 1);
        break;
    case MIP_CMD_GET_WEIGHT:
        m_lastWeight = response[1];
        m_flags |= MIP_FLAG_WEIGHT_VALID;
        break;
    case MIP_CMD_CLAP_RESPONSE:
        m_clapEvents.push(response[1]);
        break;
    case MIP_CMD_GET_DETECTED_MIP:
        m_detectedMiPEvents.push(response[1]);
        break;
    case MIP_CMD_RECEIVE_IR_DONGLE_CODE:
        for(size_t i = 0; i < length; i++)
        {
            irCode <<= 8;
            irCode |= response[i+1];
        }
        m_irCodeEvents.push(irCode);
        break;
    default:
        // Invalid notification command bytes were already handled in the previous switch so should never get here.
        MIP_ASSERT ( false );
        break;
    }
}

uint8_t MiP::discardUnexpectedSerialData()
{
    uint8_t discardedBytes = 0;

    // Unexpected response data encountered. Throw away all data in serial buffer since it is hard to tell
    // where next response begins.
    while (Serial.available() > 0)
    {
        discardedBytes++;
        Serial.read();

        // Delay long enough for next serial byte to be received if MiP is still actively sending at 115200 baud.
        delayMicroseconds(100);
    }
    return discardedBytes;
}
