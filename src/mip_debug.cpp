/* Copyright (C) 2018  Joao Lopes https://github.com/JoaoLopesF

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
/*
   This is the implementation of the MiPDebug server for outputting debug messages to telnet.
   Written by Samuel Trassare and based on Joao Lopes' original RemoteDebug library.
*/
#include "mip_debug.h"
#include <Arduino.h>

// Define a version number just for this telnet server, not the overall mip_esp8266 library.
#define VERSION "1.0.0"

// The telnet server instance.
WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;

void MiPDebug::begin(String hostname, uint8_t startingDebugLevel)
{
    telnetServer.begin();
    telnetServer.setNoDelay(true);

    // Reserve space to buffer output.
    m_bufferPrint.reserve(BUFFER_PRINT);

#ifdef CLIENT_BUFFERING
    // Reserve space for the send buffer.
    m_bufferPrint.reserve(MAX_SIZE_SEND);
#endif

    // Host name of this device.
    m_hostname = hostname;
    m_clientDebugLevel = startingDebugLevel;
    m_lastDebugLevel = startingDebugLevel;
}

void MiPDebug::stop()
{
    // Stop the client.
    if (telnetClient && telnetClient.connected())
    {
        telnetClient.stop();
    }

    // Stop the server.
    telnetServer.stop();
}

// Handle the connection.  Must be called each time through the loop().
void MiPDebug::handle()
{
#ifdef ALPHA_VERSION
    static uint32_t lastTime = millis();
#endif

    // Debug level is profiler. Set the level before.
    if (m_clientDebugLevel == PROFILER)
    {
        if (millis() > m_levelProfilerDisable)
        {
            m_clientDebugLevel = m_levelBeforeProfiler;
            if (m_connected)
            {
                telnetClient.println("* Debug level profile is now inactive.");
            }
        }
    }

#ifdef ALPHA_VERSION
    // Automatically change to profiler level if time between handles is greater than n millis.
    if (m_autoLevelProfiler > 0 &&m_clientDebugLevel != PROFILER)
    {
        uint32_t diff = (millis() - lastTime);

        if (diff >= m_autoLevelProfiler)
        {
            m_levelBeforeProfiler = m_clientDebugLevel;
            m_clientDebugLevel = PROFILER;
            m_levelProfilerDisable = 1000; // Disable it at 1 second.
            if (m_connected)
            {
                telnetClient.printf("* Debug level profile is now active - time between handles: %u\r\n", diff);
            }
        }
        lastTime = millis();
    }
#endif

    // Look for a connected client.
    if (telnetServer.hasClient())
    {

        if (telnetClient && telnetClient.connected())
        {

            // Verify if the connecting IP is same as the previous connection.

            WiFiClient newClient;
            newClient = telnetServer.available();
            String ip = newClient.remoteIP().toString();

            if (ip == telnetClient.remoteIP().toString())
            {
                // Reconnect.
                telnetClient.stop();
                telnetClient = newClient;
            }
            else
            {
                // Disconnect. Do not allow more than one connection.
                newClient.stop();

                return;
            }
        }
        else
        {
            // New TCP client.
            telnetClient = telnetServer.available();
        }
        if (!telnetClient)
        {
            // There's no client connected yet so just return.
            return;
        }

        // Set the client.  setNoDelay() offers faster execution.
        telnetClient.setNoDelay(true);

        // Clear input buffer to prevent strange characters being written to output.
        telnetClient.flush();

        // Clear the buffer.
        m_bufferPrint = "";

        // Mark now as the last time of activity.
        m_lastTimeCommand = millis();

        // Clear the current command.
        m_command = "";

        // And clear the last command.
        m_lastCommand = "";

        // Set the last print time to now.
        m_lastTimePrint = millis();

        // Show the initial help message.
        showHelp();

#ifdef CLIENT_BUFFERING
        // Client buffering - send data in intervals to avoid delays or if it is too big.
        m_bufferSend = "";
        m_sizeBufferSend = 0;
        m_lastTimeSend = millis();
#endif

        // Read the input stream.
        delay(100);
        while (telnetClient.available()) {
            telnetClient.read();
        }
    }

    // Is a client connected? Let's check to reduce overhead if there's no active connection.
    m_connected = (telnetClient && telnetClient.connected());

    // Get the user's command from telnet.
    if (m_connected)
    {
        char last = ' '; // To avoid processing the "\r\n" twice.

        while (telnetClient.available()) {

            // Get a single character.
            char character = telnetClient.read();

            // Check for a newline (CR or LF) just once time.
            if (isCRLF(character) == true)
            {
                if (isCRLF(last) == false)
                {
                    // Process the command.
                    if (m_command.length() > 0)
                    {
                        // Store the last command.
                        m_lastCommand =m_command;
                        processCommand();
                    }
                }

                // Initialize it for next command.
                m_command = "";

            }
            else if (isPrintable(character))
            {
                // Concatenate the characted to the command string.
                m_command.concat(character);
            }

            // Set this character as the last received character.
            last = character;
        }

#ifdef CLIENT_BUFFERING
        // Client buffering - send data in intervals to avoid delays or if its is too big

        if ((millis() - m_lastTimeSend) >= DELAY_TO_SEND || m_sizeBufferSend >= MAX_SIZE_SEND)
        {
            telnetClient.print(m_bufferSend);
            m_bufferSend = "";
            m_sizeBufferSend = 0;
            m_lastTimeSend = millis();
        }
#endif

#ifdef MAX_TIME_INACTIVE
        // Inactivity - close connection if no commands have been received from the user in a
        // defined interval.
        if ((millis() - m_lastTimeCommand) > MAX_TIME_INACTIVE)
        {
            telnetClient.println("* Closing session due to inactivity.");
            telnetClient.stop();
            m_connected = false;
        }
#endif
    }
}

// If the option is enabled, send debug output to serial too.
void MiPDebug::setSerialEnabled(bool enable)
{
    m_serialEnabled = enable;
    m_showColors = false;
}

// Allow the telnet client to reset the D1 mini Pack.
void MiPDebug::setResetCmdEnabled(bool enable)
{
    m_resetCommandEnabled = enable;
}

// Show time in milliseconds.
void MiPDebug::showTime(bool show)
{
    m_showTime = show;
}

// Show profiler - The time in milliseconds between debug messages.
void MiPDebug::showProfiler(bool show, uint32_t minTime)
{
    m_showProfiler = show;
    m_minTimeShowProfiler = minTime;
}

#ifdef ALPHA_VERSION
// Automatically change to profiler level if time between handles is greater than n milliseconds
// (0 - disable).
void MiPDebug::autoProfilerLevel(uint32_t millisElapsed)
{
    m_autoLevelProfiler = millisElapsed;
}
#endif

// Show the debug level.
void MiPDebug::showDebugLevel(bool show)
{
    m_showDebugLevel = show;
}

// Show colors.

void MiPDebug::showColors(bool show)
{
    if (m_serialEnabled == false)
    {
        m_showColors = show;
    }
    else
    {
        m_showColors = false; // Disable this for Serial1.
    }
}

// Is a particular debug level active?  Useful for printing messages at a desired level.
bool MiPDebug::isActive(uint8_t debugLevel)
{
    // Active -> Debug level ok and
    //           telnet connected or
    //           Serial enabled.
    bool ret = (debugLevel >= m_clientDebugLevel
            && (m_connected || m_serialEnabled));

    if (ret)
    {
       m_lastDebugLevel = debugLevel;
    }

    return ret;
}

// Set help for commands over telnet set by the user.
void MiPDebug::setHelpProjectsCmds(String help)
{
    m_helpProjectCmds = help;
}

// Set callback of sketch function to process project messages.
void MiPDebug::setCallBackProjectCmds(void (*callback)())
{
    m_callbackProjectCmds = callback;
}

// Print the user's debug message.
size_t MiPDebug::write(const uint8_t *buffer, size_t size)
{
    for(size_t i = 0; i < size; i++) {
        write((uint8_t)buffer[i]);
    }

    return size;
}

// Print the user's debug message.
size_t MiPDebug::write(uint8_t character)
{
    uint32_t elapsed = 0;
    size_t ret = 0;

    // Was a newline written earlier?
    if (m_newLine)
    {
        String show = "";

        // Show debug level if the option is turned on.
        if (m_showDebugLevel)
        {
            if (m_showColors == false)
            {
                switch (m_lastDebugLevel)
                {
                    case PROFILER:
                        show = "P";
                        break;
                    case VERBOSE:
                        show = "v";
                        break;
                    case DEBUG:
                        show = "d";
                        break;
                    case INFO:
                        show = "i";
                        break;
                    case WARNING:
                        show = "w";
                        break;
                    case ERROR:
                        show = "e";
                        break;
                }
            }
            else
            {
                // Show colors if the option is turned on.
                switch (m_lastDebugLevel)
                {
                    case PROFILER:
                        show = "P";
                        break;
                    case VERBOSE:
                        show = "v";
                        break;
                    case DEBUG:
                        show = COLOR_BACKGROUND_GREEN;
                        show.concat("d");
                        break;
                    case INFO:
                        show = COLOR_BACKGROUND_WHITE;
                        show.concat("i");
                        break;
                    case WARNING:
                        show = COLOR_BACKGROUND_YELLOW;
                        show.concat("w");
                        break;
                    case ERROR:
                        show = COLOR_BACKGROUND_RED;
                        show.concat("e");
                        break;
                    }
                if (show.length() > 1)
                {
                    show.concat(COLOR_RESET);
                }
            }
        }

        // Show time in milliseconds if the option is set.
        if (m_showTime)
        {
            if (show != "")
            {
                show.concat(" ");
            }
            show.concat("t:");
            show.concat(millis());
            show.concat("ms");
        }

        // Show profiler (time between messages) if the option is set.
        if (m_showProfiler)
        {
            elapsed = (millis() - m_lastTimePrint);
            bool resetColors = false;
            if (show != "")
            {
                show.concat(" ");
            }
            if (m_showColors)
            {
                if (elapsed < 250)
                {
                    ; // No color for this.
                }
                else if (elapsed < 1000)
                {
                    show.concat(COLOR_BACKGROUND_CYAN);
                    resetColors = true;
                }
                else if (elapsed < 3000)
                {
                    show.concat(COLOR_BACKGROUND_YELLOW);
                    resetColors = true;
                }
                else if (elapsed < 3000)
                {
                    show.concat(COLOR_BACKGROUND_MAGENTA);
                    resetColors = true;
                }
                else
                {
                    show.concat(COLOR_BACKGROUND_RED);
                    resetColors = true;
                }
            }
            show.concat("p:^");
            show.concat(formatNumber(elapsed, 4));
            show.concat("ms");
            if (resetColors)
            {
                show.concat(COLOR_RESET);
            }
            m_lastTimePrint = millis();
        }

        if (show != "")
        {
            String send = "(";
            send.concat(show);
            send.concat(") ");

            // Copy to the telnet buffer.
            if (m_connected || m_serialEnabled)
            {
                m_bufferPrint = send;
            }
        }
        m_newLine = false;
    }

    bool doPrint = false;

    // Is the current character a newline?
    if (character == '\n')
    {
        // For Windows clients.
        m_bufferPrint.concat("\r");

        m_newLine = true;
        doPrint = true;

    // If the output buffer is full then set the bool to print.
    }
    else if (m_bufferPrint.length() == BUFFER_PRINT)
    {
        doPrint = true;
    }

    // Write to the telnet buffer.
    m_bufferPrint.concat((char) character);

    // Send the buffered characters.
    if (doPrint)
    {
        bool noPrint = false;

        if (m_showProfiler && elapsed < m_minTimeShowProfiler)
        {
            noPrint = true;

        // Check the filter before printing output.
        }
        else if (m_filterActive)
        {
            String aux = m_bufferPrint;
            aux.toLowerCase();

            // No match found so don't print.
            if (aux.indexOf(m_filter) == -1)
            {
                noPrint = true;
            }
        }

        if (noPrint == false)
        {
            // Send to telnet buffer.
            if (m_connected)
            {
#ifndef CLIENT_BUFFERING
                telnetClient.print(m_bufferPrint);
#else
                uint8_t size = m_bufferPrint.length();

                // Is the buffer too big?
                if ((m_sizeBufferSend + size) >= MAX_SIZE_SEND)
                {
                    // Send it.
                    telnetClient.print(m_bufferSend);
                    m_bufferSend = "";
                    m_sizeBufferSend = 0;
                    m_lastTimeSend = millis();
                }

                // Add to send buffer.
                m_bufferSend.concat(m_bufferPrint);
                m_sizeBufferSend += size;

                // Client buffering - send data in intervals to avoid delays or if it is too big.
                if ((millis() - m_lastTimeSend) >= DELAY_TO_SEND)
                {
                    telnetClient.print(m_bufferSend);
                    m_bufferSend = "";
                    m_sizeBufferSend = 0;
                    m_lastTimeSend = millis();
                }
#endif
            }

            // Echo to serial without buffering.
            if (m_serialEnabled)
            {
                Serial1.print(m_bufferPrint);
            }
        }

        // Empty the buffer.
        ret = m_bufferPrint.length();
        m_bufferPrint = "";
    }

    return ret;
}

// Expand "CR/LF" characters to "\\r" and "\\n".
String MiPDebug::expand(String string)
{
    string.replace("\r", "\\r");
    string.replace("\n", "\\n");

    return string;
}

// This is an internal protected method that displays the help dialog.
void MiPDebug::showHelp()
{
    String help = "";

    help.concat("*** Welcome to MiP's debug terminal.  This is version ");
    help.concat(VERSION);
    help.concat(".\r\n");
    help.concat("* Hostname: ");
    help.concat(m_hostname);
    help.concat("\r\n");
    help.concat("* IP: ");
    help.concat(WiFi.localIP().toString());
    help.concat("\r\n");
    help.concat("* MAC address: ");
    help.concat(WiFi.macAddress());
    help.concat("\r\n");
    help.concat("* Free heap RAM: ");
    help.concat(ESP.getFreeHeap());
    help.concat("\r\n");
    help.concat("******************************************************\r\n");
    help.concat("* Commands:\r\n");
    help.concat("    ? or help -> display these help commands\r\n");
    help.concat("    q -> quit (close this connection)\r\n");
    help.concat("    m -> display available memory\r\n");
    help.concat("    v -> set debug level to verbose\r\n");
    help.concat("    d -> set debug level to debug\r\n");
    help.concat("    i -> set debug level to info\r\n");
    help.concat("    w -> set debug level to warning\r\n");
    help.concat("    e -> set debug level to errors\r\n");
    help.concat("    l -> show debug level\r\n");
    help.concat("    t -> show time in milliseconds\r\n");
    help.concat("    profiler:\r\n");
    help.concat("      p      -> show time between actual and last message (in millis)\r\n");
    help.concat("      p min  -> show only if time is this minimal\r\n");
    help.concat("      P time -> set debug level to profiler\r\n");
#ifdef ALPHA_VERSION
    help.concat("      A time -> set auto debug level to profiler\r\n");
#endif
    help.concat("    c -> show colors\r\n");
    help.concat("    filter:\r\n");
    help.concat("          filter <string> -> show only debug messages containing this value\r\n");
    help.concat("          nofilter        -> disable the filter\r\n");
    help.concat("    cpu80  -> Set the ESP8266 CPU to 80 MHz\r\n");
    help.concat("    cpu160 -> Set the ESP8266 CPU to 160 MHz\r\n");
    if (m_resetCommandEnabled)
    {
        help.concat("    reset -> reset the D1 mini Pack\r\n");
    }

    if (m_helpProjectCmds != "" && m_callbackProjectCmds)
    {
        help.concat("\r\n");
        help.concat("    * Project commands:\r\n");
        String show = "\r\n";
        show.concat(m_helpProjectCmds);

        // Indent this.
        show.replace("\n", "\n    ");
        help.concat(show);
    }

    help.concat("\r\n");
    help.concat(
            "* Please type the command and press enter to execute.(? or h for this help)\r\n");
    help.concat("***\r\n");

    telnetClient.print(help);
}

// This is an internal protected method to get the last command received.
String MiPDebug::getLastCommand()
{
    return m_lastCommand;
}

// This is an internal protected method to clear the last command received.

void MiPDebug::clearLastCommand()
{
    m_lastCommand = "";
}

// This is an internal protected method to process the user's command received from telnet.
void MiPDebug::processCommand()
{
    telnetClient.print("* Debug: Command received: ");
    telnetClient.println(m_command);

    String options = "";
    uint8_t pos = m_command.indexOf(" ");
    if (pos > 0)
    {
        options = m_command.substring(pos + 1);
    }

    // Set time of last command received.
    m_lastTimeCommand = millis();

    // Process the command.
    if (m_command == "h" || m_command == "?" || m_command == "help")
    {
        showHelp();
    }
    else if (m_command == "q")
    {
        // Quit.
        telnetClient.println("* Closing telnet connection ...");

        telnetClient.stop();

    }
    else if (m_command == "m")
    {
        telnetClient.print("* Free heap RAM: ");
        telnetClient.println(ESP.getFreeHeap());
    }
    else if (m_command == "cpu80")
    {
        // Change ESP8266 CPU frequency to 80 MHz.
        system_update_cpu_freq(80);
        telnetClient.println("ESP8266 CPU changed to 80 MHz");

    }
    else if (m_command == "cpu160")
    {
        // Change ESP8266 CPU frequency to 160 MHz.
        system_update_cpu_freq(160);
        telnetClient.println("ESP8266 CPU changed to 160 MHz");
    }
    else if (m_command == "v")
    {
        // Set the debug level.
        m_clientDebugLevel = VERBOSE;

        telnetClient.println("* Debug level set to Verbose");
    }
    else if (m_command == "d")
    {
        // Set the debug level.
        m_clientDebugLevel = DEBUG;

        telnetClient.println("* Debug level set to Debug");
    }
    else if (m_command == "i")
    {
        // Set the debug level.
        m_clientDebugLevel = INFO;

        telnetClient.println("* Debug level set to Info");
    }
    else if (m_command == "w")
    {
        // Set the debug level.
        m_clientDebugLevel = WARNING;

        telnetClient.println("* Debug level set to Warning");
    }
    else if (m_command == "e")
    {
        // Set the debug level.
        m_clientDebugLevel = ERROR;

        telnetClient.println("* Debug level set to Error");
    }
    else if (m_command == "l")
    {
        // Show the debug level.
        m_showDebugLevel = !m_showDebugLevel;

        telnetClient.printf("* Show debug level: %s\r\n",
                m_showDebugLevel ? "on" : "off");
    }
    else if (m_command == "t")
    {
        // Show the time.
        m_showTime = !m_showTime;

        telnetClient.printf("* Show time: %s\r\n", m_showTime ? "on" : "off");
    }
    else if (m_command == "p")
    {
        // Show the profiler status.
        m_showProfiler = !m_showProfiler;
        m_minTimeShowProfiler = 0;

        telnetClient.printf("* Show profiler: %s\r\n",
                m_showProfiler ? "on" : "off");
    }
    else if (m_command.startsWith("p "))
    {
        // Show profiler with minimal time.
        if (options.length() > 0)
        {
            int32_t aux = options.toInt();
            if (aux > 0)
            {
                m_showProfiler = true;
                m_minTimeShowProfiler = aux;
                telnetClient.printf(
                        "* Show profiler: on (with minimal time: %u)\r\n",
                       m_minTimeShowProfiler);
            }
        }
    }
    else if (m_command == "P")
    {
        // Debug level profile.
        m_levelBeforeProfiler =m_clientDebugLevel;
        m_clientDebugLevel = PROFILER;

        if (m_showProfiler == false)
        {
            m_showProfiler = true;
        }

        // Default of 1 second.
        m_levelProfilerDisable = 1000;

        if (options.length() > 0)
        {
            int32_t aux = options.toInt();
            if (aux > 0)
            {
                m_levelProfilerDisable = millis() + aux;
            }
        }

        telnetClient.printf(
                "* Debug level set to Profiler (disable in %u millis)\r\n",
               m_levelProfilerDisable);
    }
    else if (m_command == "A")
    {
        // Auto debug level profile.  Default of 1 second.
        m_autoLevelProfiler = 1000;

        if (options.length() > 0)
        {
            int32_t aux = options.toInt();
            if (aux > 0)
            {
                m_autoLevelProfiler = aux;
            }
        }

        telnetClient.printf(
                "* Auto profiler debug level active (time >= %u millis)\r\n",
               m_autoLevelProfiler);
    }
    else if (m_command == "c")
    {
        // Show status of colors.
        m_showColors = !m_showColors;

        telnetClient.printf("* Show colors: %s\r\n",
                m_showColors ? "on" : "off");
    }
    else if (m_command.startsWith("filter ") && options.length() > 0)
    {
        setFilter(options);
    }
    else if (m_command == "nofilter")
    {
        setNoFilter();
    }
    else if (m_command == "reset" && m_resetCommandEnabled)
    {
        telnetClient.println("* Reset...");

        telnetClient.println("* Closing telnet connection...");

        telnetClient.println("* Resetting the D1 mini Pack...");

        telnetClient.stop();
        telnetServer.stop();

        delay(500);

        ESP.restart();
    }
    else
    {
        // Project commands - set by the user.

        if (m_callbackProjectCmds)
        {
            m_callbackProjectCmds();
        }
    }
}

// This is an internal protected method to set a filter.
void MiPDebug::setFilter(String filter)
{
    m_filter = filter;
    m_filter.toLowerCase();
    m_filterActive = true;

    telnetClient.print("* Debug: Filter active: ");
    telnetClient.println(m_filter);
}

// This is an internal protected method to remove a filter.
void MiPDebug::setNoFilter()
{
    m_filter = "";
    m_filterActive = false;

    telnetClient.println("* Debug: Filter disabled");
}

// This is an internal protected method to format numbers.
String MiPDebug::formatNumber(uint32_t value, uint8_t size, char insert)
{
    // Pad zeroes to the left.
    String ret = "";

    for (uint8_t i = 1; i <= size; i++) {
        uint32_t max = pow(10, i);
        if (value < max)
        {
            for (uint8_t j = (size - i); j > 0; j--) {
                ret.concat(insert);
            }
            break;
        }
    }

    ret.concat(value);

    return ret;
}

// This is an internal protected method to determine if a character is a carriage return or
// line feed.
bool MiPDebug::isCRLF(char character)
{
    return (character == '\r' || character == '\n');
}
