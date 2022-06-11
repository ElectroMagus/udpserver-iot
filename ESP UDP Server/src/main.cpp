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

/*  Pin Defs for Motor Driver
// Robot Motors
int motor1a = 12;
int motor1b = 27;
int motor2a = 33;
int motor2b = 15;
*/

#include <MX1508.h>                     // See lib for source code.  Adapted for ESP32 from the AVR library at https://github.com/Saeterncj/MX1508
// Create motor object for each wheel
MX1508 motorA(12, 27, 0, 1);       //  Pin1, Pin2, PWMChannel 1, PWMChannel 2
MX1508 motorB(33, 15, 2, 3);

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
        //stopRob();
        motorA.stopMotor();
        motorB.stopMotor();
      } else if (commandData == 1) {
        //forwardRob();
        motorA.motorGo(200);
        motorB.motorGo(200);
      } else if (commandData == 2) {
        motorA.motorRev(200);
        motorB.motorRev(200);
        //reverseRob();
      } else if (commandData == 3) {
        //if (speedR <= 250) { speedR = speedR + 5; }
        //if (speedL <= 250) { speedL = speedL + 5; }
      } else if (commandData == 4) {
       // if (speedR >= 0) { speedR = speedR - 5; }
       // if (speedL >= 0) { speedL = speedL - 5; }
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
ArduinoOTA.handle();    // TODO:  Move to own task and kill loop from running
vTaskDelay(1);          //  Removed delay() since we're using tasks and want to let loop() yield

}
