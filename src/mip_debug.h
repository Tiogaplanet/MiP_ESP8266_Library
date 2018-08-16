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
   This header file describes the public API for sending debug messages to a telnet client.
   Written by Samuel Trassare and based on Joao Lopes' original RemoteDebug library.
 */
#ifndef MIPDEBUG_H
#define MIPDEBUG_H

#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "Print.h"

extern "C" {
bool system_update_cpu_freq(uint8 freq);
}

// Define an mechanism for quickly calling the various debug levels provided by the system.
#define DEBUG(...)   { if (Debug.isActive(Debug.ANY)) Debug.printf(__VA_ARGS__); }
#define DEBUG_P(...) { if (Debug.isActive(Debug.PROFILER)) Debug.printf(__VA_ARGS__); }
#define DEBUG_V(...) { if (Debug.isActive(Debug.VERBOSE)) Debug.printf(__VA_ARGS__); }
#define DEBUG_D(...) { if (Debug.isActive(Debug.DEBUG)) Debug.printf(__VA_ARGS__); }
#define DEBUG_I(...) { if (Debug.isActive(Debug.INFO)) Debug.printf(__VA_ARGS__); }
#define DEBUG_W(...) { if (Debug.isActive(Debug.WARNING)) Debug.printf(__VA_ARGS__); }
#define DEBUG_E(...) { if (Debug.isActive(Debug.ERROR)) Debug.printf(__VA_ARGS__); }

// The default port for the telnet service.
#define TELNET_PORT 23

// Maximum time for inactivity (in milliseconds). Comment this line to disable the timeout interval.
// Default: 10 minutes
#define MAX_TIME_INACTIVE 600000

// Defines the buffer size for buffered output.
#define BUFFER_PRINT 150

// ANSI color codes.
#define COLOR_RESET "\x1B[0m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW "\x1B[0;33m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"
#define COLOR_DARK_BLACK "\x1B[1;30m"
#define COLOR_DARK_RED "\x1B[1;31m"
#define COLOR_DARK_GREEN "\x1B[1;32m"
#define COLOR_DARK_YELLOW "\x1B[1;33m"
#define COLOR_DARK_BLUE "\x1B[1;34m"
#define COLOR_DARK_MAGENTA "\x1B[1;35m"
#define COLOR_DARK_CYAN "\x1B[1;36m"
#define COLOR_DARK_WHITE "\x1B[1;37m"
#define COLOR_BACKGROUND_BLACK "\x1B[40m"
#define COLOR_BACKGROUND_RED "\x1B[41m"
#define COLOR_BACKGROUND_GREEN "\x1B[42m"
#define COLOR_BACKGROUND_YELLOW "\x1B[43m"
#define COLOR_BACKGROUND_BLUE "\x1B[44m"
#define COLOR_BACKGROUND_MAGENTA "\x1B[45m"
#define COLOR_BACKGROUND_CYAN "\x1B[46m"
#define COLOR_BACKGROUND_WHITE "\x1B[47m"



class MiPDebug: public Print
{
public:
    void begin(String hostname, uint8_t startingDebugLevel = DEBUG);
    void stop();

    void handle();
    void setSerialEnabled(bool enable);
    void setResetCmdEnabled(bool enable);
    void setHelpProjectsCmds(String help);
    void setCallBackProjectCmds(void (*callback)());
    String getLastCommand();
    void clearLastCommand();

    void showTime(bool show);
    void showProfiler(bool show, uint32_t minTime = 0);
    void showDebugLevel(bool show);
    void showColors(bool show);

    void autoProfilerLevel(uint32_t millisElapsed);

    void setFilter(String filter);
    void setNoFilter();

    bool isActive(uint8_t debugLevel = DEBUG);

    // This is the extended write function for the Print class.
    virtual size_t write(uint8_t);

    // Definitions for each of the debug levels.
    static const uint8_t PROFILER = 0;  // Used to show time of execution of pieces of code (profiler).
    static const uint8_t VERBOSE = 1;   // Used to show verbose messages.
    static const uint8_t DEBUG = 2;     // Used to show debug messages.
    static const uint8_t INFO = 3;      // Used to show info messages.
    static const uint8_t WARNING = 4;   // Used to show warning messages.
    static const uint8_t ERROR = 5;     // Used to show error messages.
    static const uint8_t ANY = 6;       // Used to show messages at any debug level.

    // Expand "CR/LF" characters to "\\r" and "\\n".
    String expand(String string);

protected:
    void   showHelp();
    void   processCommand();
    String formatNumber(uint32_t value, uint8_t size, char insert='0');
    bool   isCRLF(char character);

    String   m_hostname = "";               // The user-defined hostname for the telnet server.
    bool     m_connected = false;           // Is a client connected?
    uint8_t  m_clientDebugLevel = DEBUG;    // The debug level set by the user in telnet.
    uint8_t  m_lastDebugLevel = DEBUG;      // Last debug level set by active().
    uint32_t m_lastTimePrint = millis();    // The last time a line was printed.
    uint8_t  m_levelBeforeProfiler = DEBUG; // Last level before setting the profiler level.
    uint32_t m_levelProfilerDisable = 0;    // Time in millis to disable the profiler level.
    uint32_t m_autoLevelProfiler = 0;       // Automatic change to profiler level if time between handles is greater than n millis
    bool     m_showTime = false;            // Show time in milliseconds.
    bool     m_showProfiler = false;        // Show time between messages.
    uint32_t m_minTimeShowProfiler = 0;     // Minimum time to show profiler.
    bool     m_showDebugLevel = true;       // Show debug level on each debug message.
    bool     m_showColors = false;          // Show colors.
    bool     m_serialEnabled = false;       // Send debug messages to serial too.
    bool     m_resetCommandEnabled = false; // Allow the telnet server to reset the ESP8266.
    bool     m_newLine = true;              // New line write ?
    String   m_command = "";                // The current command received from the user.
    String   m_lastCommand = "";            // The last command received from the user.
    uint32_t m_lastTimeCommand = millis();  // Time that the last command was received.
    String   m_helpProjectCmds = "";        // Help commands set by the project (sketch).
    void     (*m_callbackProjectCmds)();    // Callable for project commands.
    String   m_filter = "";                 // The filter string.
    bool     m_filterActive = false;        // Is the filter active?
    String   m_bufferPrint = "";            // Print buffer for telnet output.
};

#endif
