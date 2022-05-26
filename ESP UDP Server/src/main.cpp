#include <Arduino.h>


// OTA Update Libraries
#include <WiFi.h>
#include <ESPmDNS.h>          // This library isn't availible in PlatformIO's library tool, and has to be manually put into the lib directory. It's contained in the Arduino ESP32 framework.
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ssid = "";
const char* password = "";

// UDP Libraries
#include "AsyncUDP.h"
AsyncUDP udp;

void setup() {
  Serial.begin(115200);
// OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  ArduinoOTA.setHostname("espserver");                // mDNS hostname
  ArduinoOTA.begin();


   // Setup UDP Server to listen
  if(udp.listen(20001)) {
    // When a packet is recieved
    udp.onPacket([](AsyncUDPPacket packet) {
      // Copies the packet data from memory into a string for parsing.   
      // packet.data is unit8_t *, so possibly do a comparison directly vs the memcpy to speed up the transaction
      char* tmpStr = (char*) malloc(packet.length() + 1);
      memcpy(tmpStr, packet.data(), packet.length());
      tmpStr[packet.length()] = '\0'; // ensure null termination
      String header = String(tmpStr);
      free(tmpStr); // Free up that memory

      // Using a leading F as a header for testing       
      if (header[0] == 'F') {
        int select = header[1] - '0';     // Converts the string to an integer during declaration to make the compiler happy
        //sweepPos(select);
        //sweepNeg(select);
        packet.printf("OK - %i", select);
      } else

      // All other packets, just return the header to the client for diagnostics
        packet.printf("Header Recieved: %c", header[0]);
    });
  }
}



void loop() {


ArduinoOTA.handle();
delay(1);

}