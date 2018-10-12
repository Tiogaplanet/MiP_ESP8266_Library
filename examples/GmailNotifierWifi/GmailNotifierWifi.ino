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
// This example sketch shows the bare minimum needed to connect MiP to wifi.
// This sketch may be used as a starting point for your sketch.
#include <mip_esp8266.h>
#include <WiFiClientSecure.h>   // Include the HTTPS library

//const char* ssid = "..............";                // Enter the SSID for your wifi network.
//const char* password = "..............";            // Enter your wifi password.

const char* hostname = "MiP-0x01";                  // Set any hostname you desire.

MiP         mip;                              // We need a single MiP object
bool        connectResult;                    // Test whether a connection to MiP was established.

const char* host = "mail.google.com"; // the Gmail server
const char* url = "/mail/feed/atom";  // the Gmail feed url
const int httpsPort = 443;            // the port to connect to the email server

// The SHA-1 fingerprint of the SSL certificate for the Gmail server (see below)
const char* fingerprint = "D3 90 FC 82 07 E6 0D C2 CE F9 9D 79 7F EC F6 E6 3E CB 8B B3";

// The Base64 encoded version of your Gmail login credentials (see below)
const char* credentials = "ZW1haWwuYWRkcmVzc0BnbWFpbC5jb206cGFzc3dvcmQ=";

void setup() {
  connectResult = mip.begin(ssid, password, hostname);

  if (!connectResult) {
    Serial1.println(F("Failed connecting to MiP."));
    return;
  }

  Serial1.print(F("IP address: "));               // You could delete this chunk of code.  It's just here
  Serial1.println(WiFi.localIP());             // to show your IP address.
}


void loop() {
  ArduinoOTA.handle();                        // Without this we can't do OTA programming.

  int unread = getUnread();
  if (unread == 0) {
    Serial1.println(F("\r\nYou've got no unread emails"));
    mip.writeChestLED(0x00, 0xFF, 0x00);  // Set chest to solid green.
  } else if (unread > 0) {
    Serial1.printf("\r\nYou've got %d new messages\r\n", unread);
    mip.writeChestLED(0x00, 0xFF, 0x00, 990, 980)
  } else {
    Serial1.println(F("Could not get unread mails"));
  }
  Serial1.println(F('\n'));
  delay(5000);
}

int getUnread() {    // a function to get the number of unread emails in your Gmail inbox
  WiFiClientSecure client; // Use WiFiClientSecure class to create TLS (HTTPS) connection
  Serial1.printf("Connecting to %s:%d ... \r\n", host, httpsPort);
  if (!client.connect(host, httpsPort)) {   // Connect to the Gmail server, on port 443
    Serial1.println(F("Connection failed"));    // If the connection fails, stop and return
    return -1;
  }

  if (client.verify(fingerprint, host)) {   // Check the SHA-1 fingerprint of the SSL certificate
    Serial1.println(F("Certificate matches"));
  } else {                                  // if it doesn't match, it's not safe to continue
    Serial1.println(F("Certificate doesn't match"));
    return -1;
  }

  Serial1.print(F("Requesting URL: "));
  Serial1.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Basic " + credentials + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n"); // Send the HTTP request headers

  Serial1.println(F("Request sent"));

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
  Serial1.println("Connection closed");

  return unread;                                        // Return the number of unread emails
}

