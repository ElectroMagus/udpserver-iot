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


// MX1508 Library from:  https://github.com/ElectroMagus/ESP32MX1508.git
#include <ESP32MX1508.h>                     
// Create motor object for each wheel  (Motor 1-  12,27   Motor 2- 33,15)
MX1508 motorA(12, 27, 11, 12, 16, 5000);       //  Pin1, Pin2, PWMChannel 1, PWMChannel 2   (16 Channels availible 0-15), resolution (8,12,16), frequency Hz
MX1508 motorB(15, 33, 13, 14, 16, 5000);
uint16_t speedL = 52000;               // Set default speed
uint16_t speedR = 53000;               // Set default speed

// FreeRTOS Task and Queue Handle Declarations
QueueHandle_t CommQ;   // Queue for commands
TaskHandle_t procCommQ;  // Handler for task to process commands
TaskHandle_t distanceH; // Handler for distance sensor readings

// Function used for measuring distance and acting as a basic autonomous mode.   
void distanceSensorfunc(void *pvParameters) {
  for(;;) {		// Run forever unless killed by task handle
          // Placeholder
     }
}

// Function used in the procCommQ Task to process the CommQ Queue
void procCommQfunc(void *pvParameters) {
  for(;;) {		// Run forever unless killed by task handle
    int commandData;   // variable used to process the next command on the command queue
    for(int i = 0; i<20; i++){		// Make sure to select a value according to your queue size
      // Print all items currently in the command queue to the serial port
      xQueueReceive(CommQ, &commandData, portMAX_DELAY);
      //Serial.print(commandData);
      if (commandData == 0) { 
        //stopRob();
        motorA.stopMotor();
        motorB.stopMotor();
      } else if (commandData == 1) {
        //forwardRob();
        motorA.motorGo(speedR);
        motorB.motorGo(speedL);
      } else if (commandData == 2) {
        //reverseRob();
        motorA.motorRev(speedR);
        motorB.motorRev(speedL);
      } else if (commandData == 3) {
        //turnRight();
        motorA.motorRev(speedR);
        motorB.motorGo(speedL);
      } else if (commandData == 4) {
        //turnLeft();
        motorA.motorGo(speedR);
        motorB.motorRev(speedL);
      } else if (commandData == 5) {
        vTaskDelete(distanceH);
        // Suspend Autonomous mode  -- Deletes task.   Suspending caused wierd issues on resuming.                         
        motorA.stopMotor();
        motorB.stopMotor();
      } else if (commandData == 6) {
        // Start Autonomous mode  -- Creates task.  
        xTaskCreatePinnedToCore(distanceSensorfunc, "distanceSensor", 13000, NULL, 2, &distanceH, 1);      
      } else if (commandData == 7) {
        //faster();                     //  Todo:  Use map() function to define stepped speed control that is configurable per motor
        speedR = 240;
        speedL = 240;
      } else if (commandData == 8) {
        //slower();
        speedR = 200;
        speedL = 200;     
      } else if (commandData == 9) {
        //servo test
        
        
      } else
      // This section runs regardless of the commandData sent
      Serial.print(commandData);      
    }
  }
}



void otatask(void *pvParameters) {
  for(;;) {		// Run forever unless killed by task handle
      ArduinoOTA.handle();    
      vTaskDelay(1);                                    // Required to allow the UDP server to process commands
  }
}

void udpserver(void *pvParameters) {
for(;;) {		// Run forever unless killed by task handle
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
        xQueueSend(CommQ, &select, portMAX_DELAY);   // Add's the int after the F header to the FreeRTOS Command Queue for processing/scheduling to free up the UDP connection 
        packet.printf("OK - %i", select);
      } else

      // All other packets, just return the header to the client for diagnostics
      packet.printf("Header Recieved: %c", header[0]);     
    });
  }
}
}

void setup() {
  Serial.begin(115200);
  // OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  ArduinoOTA.setHostname("wheelie");                // mDNS hostname
  ArduinoOTA.begin();

  // FreeRTOS Queue Setup
  CommQ = xQueueCreate( 20, sizeof( int ) );  // args:  # of items, size in byte per item      
  if(CommQ == NULL) {
    Serial.println("Error creating the queue");
    }

 //// Create tasks for operational logic  
  // Motor Command Task 
  xTaskCreatePinnedToCore(
      procCommQfunc, // Function to implement the task 
      "procCommQ", // Name of the task 
      10000,  // Stack size in words 
      NULL,  // Task input parameter 
      2,  // Priority of the task                   // Don't run at 0 and loop() run's at 1.
      &procCommQ,  // Task handle. 
      1); // Core where the task should run 
  
  // To-do:    - Add command to pause procCommQ to allow sequence of actions to be buffered then launched on command.
  //              - Add command for pause(ms)  to add to queue
  //              - Add command to return distance reading

  // Use this to test the size of the stack on a task to adjust during creation
  // Serial.println( "Task stack high watermark %d",  uxTaskGetStackHighWaterMark( NULL ) );


// Tasks that should run all the time
  xTaskCreatePinnedToCore(otatask, "OTA", 10000, NULL, 2, NULL, 1);    // ArduinoOTA Task
  xTaskCreatePinnedToCore(udpserver, "UDP", 10000, NULL, 2, NULL, 1);  // UDP Server Task

}



void loop() {
    vTaskDelete(NULL);        //  Kill loop since everything is handled in tasks and this take up ticks
}
