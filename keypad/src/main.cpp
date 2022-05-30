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
    
    if (evt.bit.NUM < 5) {                                                        // If Num 0-4, send UPD packet with #
      if(udp.connect(IPAddress(10,9,8,142), 20001)) { 
        udp.printf("F%i", evt.bit.NUM);                                           
        } 
      } else if (evt.bit.NUM == 12) {
        colorWipe();
      }
   
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {                    // Key released
      trellis.pixels.setPixelColor(evt.bit.NUM, 0);                           // Turn color off on release
  }
  trellis.pixels.show();                                                      // Send neopixel color update
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

  // Show Ready 
  colorWipe();
}

void loop() {
  trellis.read();                 // interrupt management handles keypresses
  delay(20);                      //the trellis has a resolution of around 60hz
  ArduinoOTA.handle();


//    if(udp.connect(IPAddress(10,9,8,114), 20001)) { 
//        udp.print("D");

  
}
