#include "Arduino.h"
// OTA Update Libraries
#include <ESP8266Wifi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ssid = "";
const char* password = "";

// UDP Libraries
#include "ESPAsyncUDP.h"
AsyncUDP udp;

#include "Adafruit_NeoTrellis.h"
#include "SPI.h"
Adafruit_NeoTrellis trellis;

//  Callback for the keys selected for remote control functions
TrellisCallback remoteF(keyEvent evt){
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {                              // Key pressed
      // Send UDP Packet Based on Button Press
      if(udp.connect(IPAddress(10,9,8,77), 20001)) { 
        if (evt.bit.NUM == 14) { 
          udp.println("F1");                                           
        } else if (evt.bit.NUM == 10) {
          udp.println("F0");
        } else if (evt.bit.NUM == 6) {
          udp.println("F2");
        } else if (evt.bit.NUM == 11) {
          udp.println("F3");
        } else if (evt.bit.NUM == 9) {
          udp.println("F4");
        } else if (evt.bit.NUM == 1) {
          udp.println("F5");
        }else if (evt.bit.NUM == 0) {
          udp.println("F6");
        } else if (evt.bit.NUM == 12) {
          udp.println("F7");
        } else if (evt.bit.NUM == 8) {
          udp.println("F8");
        }
        // Anything here runs on any keypress from remote control buttons
        //
      } 
      // End Network Code
    } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {                        // On button release
       // Send stop signal on key release
       if (evt.bit.NUM == 14) { 
          udp.println("F0"); 
        } else if (evt.bit.NUM == 11) {
          udp.println("F0");
        } else if (evt.bit.NUM == 9) {
          udp.println("F0"); 
        } else if (evt.bit.NUM == 6) {
          udp.println("F0");
        }            
    }
  return 0;
}

void setup() {
  Serial.begin(115200);

  // OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  ArduinoOTA.setHostname("trellis");                // mDNS hostname
  ArduinoOTA.begin();
  
  // Initialize Keypad
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1) delay(1);
  }
  // Activate all keys and set rising and falling edge inturrupts
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
  }

  // Setup Buttons used for Remote Control Functions.   remoteF() is called on press/release
  trellis.registerCallback(14, remoteF);
  trellis.pixels.setPixelColor(14, 0x0000FF);     // 0x0000FF Blue 
  trellis.registerCallback(12, remoteF);
  trellis.pixels.setPixelColor(12, 0x00FF00);     // #00FF00 Green
  trellis.registerCallback(11, remoteF);
  trellis.pixels.setPixelColor(11, 0x0000FF);     
  trellis.registerCallback(10, remoteF);
  trellis.pixels.setPixelColor(10, 0xFF0000);     // 0xFF0000 Red
  trellis.registerCallback(8, remoteF);
  trellis.pixels.setPixelColor(8, 0xFFFF00);      // 0xFFFF00 Yellow
  trellis.registerCallback(9, remoteF);
  trellis.pixels.setPixelColor(9, 0x0000FF);     
  trellis.registerCallback(6, remoteF);
  trellis.pixels.setPixelColor(6, 0x0000FF);
  trellis.registerCallback(1, remoteF);
  trellis.pixels.setPixelColor(1, 0xFF0000);
  trellis.registerCallback(0, remoteF);
  trellis.pixels.setPixelColor(0, 0x0000FF);

  trellis.pixels.show();
  
}

void loop() {
  trellis.read();                 // interrupt management handles keypresses
  delay(20);                      //the trellis has a resolution of around 60hz
  ArduinoOTA.handle();
}
