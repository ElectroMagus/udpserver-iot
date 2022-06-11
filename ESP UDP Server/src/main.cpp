#include <Arduino.h>

// OTA Update Libraries
#include <WiFi.h>
#include <ESPmDNS.h>          
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ssid = "";
const char* password = "";

// UDP Libraries
#include "AsyncUDP.h"
AsyncUDP udp;

// Robot Motors
int motor1a = 12;
int motor1b = 27;
int motor2a = 33;
int motor2b = 15;
int speedR = 225;   // Default Speed of Right Wheels 0-255
int speedL = 225;   // Default Speed of Left Wheels 0-255


void reverseRob() {
  ledcWrite(1, 0);            
  ledcWrite(2, 0);
  ledcWrite(3, speedR);     
  ledcWrite(4, speedR);
}

void forwardRob() {
  ledcWrite(1, speedR);
  ledcWrite(2, speedL);
  ledcWrite(3, 0);
  ledcWrite(4, 0);
}

void stopRob() {
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(4, 0);
}



// FreeRTOS Tasks and Queue Declarations
QueueHandle_t commandQ;   // Queue for commands to be executed
TaskHandle_t addCommand;  // Task handler function to add items to the queue
TaskHandle_t pcommandQ;  // Task handler function to remove items from the queue

// Function used in the pcommandQ Task to process the commandQ Queue
void pcommandQfunc(void *pvParameters) {
  for(;;) {		// Run forever unless killed by task handle
    int commandData;   // variable used to process the next command on the command queue
    for(int i = 0; i<20; i++){		// Make sure to select a value according to your queue size
      // Print all items currently in the command queue to the serial port
      xQueueReceive(commandQ, &commandData, portMAX_DELAY);
      Serial.print(commandData);
      Serial.print("*");
      if (commandData == 0) { 
        stopRob();
      } else if (commandData == 1) {
        forwardRob();
      } else if (commandData == 2) {
        reverseRob();
      } else if (commandData == 3) {
        if (speedR <= 250) { speedR = speedR + 5; }
        if (speedL <= 250) { speedL = speedL + 5; }
      } else if (commandData == 4) {
        if (speedR >= 0) { speedR = speedR - 5; }
        if (speedL >= 0) { speedL = speedL - 5; }
      } else
      Serial.print(commandData);
      Serial.print("|");
    }
  }
}


void setup() {
  Serial.begin(115200);
  // OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  ArduinoOTA.setHostname("robot");                // mDNS hostname
  ArduinoOTA.begin();
  

  // Motor Setup
  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);
  // analogWrite() dosen't work with ESP32, so setting up the ledc PWM channel to send signals to the pins
  // LED Channel 1 (motor1a), 50kHz, 8 bit (0-255) resolution
  ledcSetup(1, 50000, 8);     
  ledcAttachPin(motor1a, 1);
  // LED Channel 2 (motor2a), 50kHz, 8 bit (0-255) resolution 
  ledcSetup(2, 50000, 8);     
  ledcAttachPin(motor2a, 2);  
  // LED Channel 3 (motor1b), 50kHz, 8 bit (0-255) resolution
  ledcSetup(3, 50000, 8);     
  ledcAttachPin(motor1b, 3);
  // LED Channel 4 (motor2b), 50kHz, 8 bit (0-255) resolution 
  ledcSetup(4, 50000, 8);     
  ledcAttachPin(motor2b, 4);  
  

  // FreeRTOS Queue Setup
  commandQ = xQueueCreate( 20, sizeof( int ) );  // args:  # of items, size in byte per item      
  if(commandQ == NULL) {
    Serial.println("Error creating the queue");
    }

  // Create tasks to process commandQ  
  xTaskCreatePinnedToCore(
      pcommandQfunc, // Function to implement the task 
      "pcommandQ", // Name of the task 
      10000,  // Stack size in words 
      NULL,  // Task input parameter 
      2,  // Priority of the task                   // Don't run at 0 and loop() run's at 1.
      &pcommandQ,  // Task handle. 
      1); // Core where the task should run 
	  
	    


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
        xQueueSend(commandQ, &select, portMAX_DELAY);   // Add's the int after the F header to the FreeRTOS Command Queue for processing/scheduling to free up the UDP connection 
        packet.printf("OK - %i", select);
      } else

      // All other packets, just return the header to the client for diagnostics
      packet.printf("Header Recieved: %c", header[0]);
      
    });
  }
  
}



void loop() {



ArduinoOTA.handle();
vTaskDelay(1);          //  Removed delay() since we're using tasks and want to let loop() yield
//forwardRob();



}
