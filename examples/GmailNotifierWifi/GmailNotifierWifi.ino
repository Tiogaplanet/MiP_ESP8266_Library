/* Copyright (C) 2018  Samuel Trassare (https://github.com/tiogaplanet)

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
// This example sketch checks your Gmail account and flashes MiP's chest LED
// to indicate new email.
#include <mip_esp8266.h>
#include <WiFiClientSecure.h>   // Include the HTTPS library

const char* ssid = "..............";
const char* password = "..............";

const char* hostname = "MiP-0x01";

MiP  mip;
bool connectResult;

// The Gmail server.
const char* host = "mail.google.com"; 

// The Gmail feed URL.
const char* url = "/mail/feed/atom"; 

 // The port to connect to the email server.
const int httpsPort = 443;            

// The Base64 encoded version of your Gmail login credentials.
const char* credentials = "bWlwQGdtYWlsLmNvbTpBIHZlcnkgbG9uZyBwYXNzd29yZCwgaW5kZWVkLg==";

// Keep track of new and older, unread emails.
int latestUnread;
int lastUnread;

// Store the last time Gmail was queried.
unsigned long previousMillis = 0;

// Check mail every minute (60000 milliseconds).
const long mailInterval = 60000;

// Don't let MiP disconnect from the ESP8266.  Send a read command every nine minutes.
const long keepAliveInterval = 540000;

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  // Check email for the first time.
  latestUnread = lastUnread = getUnread();
  previousMillis = millis();

  // Make sure the chest is solid before we loop.
  mip.writeChestLED(0x00, 0xFF, 0x00);
}


void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis  = millis();
  unsigned long keepAliveMillis = currentMillis;

  if (currentMillis - previousMillis >= mailInterval) {
    latestUnread = getUnread();
    if (latestUnread > lastUnread) {
      Serial1.printf("You have %d new email%s and %d older, unread email%s.\n",
                     latestUnread - lastUnread, latestUnread - lastUnread == 1 ? "" : "s", lastUnread, lastUnread == 1 ? "" : "s");
      MiPChestLED chestLED;
      mip.readChestLED(chestLED);
      if (chestLED.offTime != 980) {
        // flash the chest to indicate new email.
        mip.writeChestLED(0x00, 0xFF, 0x00, 990, 980);
      }
    } else if (latestUnread < lastUnread) {
      // User either read or deleted unread emails, so stop indicating new email.
      Serial1.printf("You have %d unread message%s.\r\n", latestUnread, latestUnread == 1 ? "" : "s");
      // Set the chest LED to solid green.
      mip.writeChestLED(0x00, 0xFF, 0x00);
    } else if (latestUnread == -1) {
      Serial1.println(F("I could not access your email. I'll try again in a minute."));
    }
    lastUnread = latestUnread;
    previousMillis = currentMillis;
  }

  if (currentMillis - keepAliveMillis >= keepAliveInterval) {
    mip.readPosition();
    keepAliveMillis = currentMillis;
  }
}

// Get the number of unread emails in your Gmail inbox.
int getUnread() {
  // Use WiFiClientSecure class to create a TLS (HTTPS) connection.
  WiFiClientSecure client;
  Serial1.printf("Connecting to %s:%d ... \r\n", host, httpsPort);
  // Connect to the Gmail server on port 443.
  if (!client.connect(host, httpsPort)) {
    // If the connection fails, stop and return.
    Serial1.println(F("Connection failed."));
    return -1;
  }

  Serial1.printf("Requesting URL: %s%s\n", host, url);

  // Send the HTTP request headers.
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Basic " + credentials + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial1.println(F("Request sent."));

  int unread = -1;

  while (client.connected()) {                          // Wait for the response. The response is in XML format
    client.readStringUntil('<');                        // read until the first XML tag
    String tagname = client.readStringUntil('>');       // read until the end of this tag to get the tag name
    if (tagname == "fullcount") {                       // if the tag is <fullcount>, the next string will be the number of unread emails
      String unreadStr = client.readStringUntil('<');   // read until the closing tag (</fullcount>)
      unread = unreadStr.toInt();                       // convert from String to int
      break;                                            // stop reading
    }                                                   // if the tag is not <fullcount>, repeat and read the next tag
  }
  Serial1.println(F("Connection closed."));

  return unread;
}

