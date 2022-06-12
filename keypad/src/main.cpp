#include "Arduino.h"
// OTA Update Libraries
#include <ESP8266Wifi.h>
//#include <ESPmDNS.h>          
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
//// Included Trellis functions from Adafruit for easy use
// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}
void colorWipe() {
  // Pretty animation  --  Tweaked from the Adafruit Example
  uint8 rand = random(9);                                     //  Adds a little starting pattern variation each time the function runs
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map((i + rand), 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(40);
  }
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(40);
  }
}
// Callback function that lights up all keys when pressed.   Added in UDP code to send a packet on each keypress in the format to speak with the ESP module controlling servos.
TrellisCallback blink(keyEvent evt){
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {                              // Key pressed
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255)));      //  Set color on press
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {                    // Key released
      trellis.pixels.setPixelColor(evt.bit.NUM, 0);                           // Turn color off on release
  }
  trellis.pixels.show();                                                      // Send neopixel color update
  return 0;
}
///// End included Adafruit functions


//  Callback for the keys selected for remote control functions
TrellisCallback remoteF(keyEvent evt){
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {                              // Key pressed
      //trellis.pixels.setPixelColor(evt.bit.NUM, 0);                             // Turn off light on keypress
      // Send UDP Packet Based on Button Press
      if(udp.connect(IPAddress(10,9,8,142), 20001)) { 
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
       if ((evt.bit.NUM == 9) || (evt.bit.NUM == 11) || (evt.bit.NUM == 14) || (evt.bit.NUM == 6) || (evt.bit.NUM == 0)) {       // These buttons should be blue
       // trellis.pixels.setPixelColor(evt.bit.NUM, 0x0000FF);                      
       } else {                                                                     // Others are red
        // trellis.pixels.setPixelColor(evt.bit.NUM, 0xFF0000);             
       }       
       // Testing sending stop signal on key release
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

  //trellis.pixels.show();
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
  } else {
    Serial.println("NeoPixel Trellis started");
  }

  // Activate all keys and set their callback to the blink function
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  // Setup Buttons used for Remote Control Functions.   remoteF() is called on press/release
  trellis.registerCallback(14, remoteF);
  trellis.pixels.setPixelColor(14, 0x0000FF);     // 0x0000FF Blue - Go
  trellis.registerCallback(12, remoteF);
  trellis.pixels.setPixelColor(12, 0x00FF00);     // Green #00FF00 - Faster
  trellis.registerCallback(11, remoteF);
  trellis.pixels.setPixelColor(11, 0x0000FF);     
  trellis.registerCallback(10, remoteF);
  trellis.pixels.setPixelColor(10, 0xFF0000);     // Red 0xFF0000 - Stop
  trellis.registerCallback(8, remoteF);
  trellis.pixels.setPixelColor(8, 0xFFFF00);     // Yellow 0xFFFF00 - Faster
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
