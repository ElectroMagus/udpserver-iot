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

// Sharp IR Sensor Library from https://github.com/NuwanJ/ESP32SharpIR.git
#include <ESP32SharpIR.h>
ESP32SharpIR sensor( ESP32SharpIR::GP2Y0A21YK0F, 32);     // Must use pin on ADC1 else will crash when Wifi is used
uint8_t readingDistance;       

// See lib for source code.  Adapted for ESP32 from the AVR library at https://github.com/Saeterncj/MX1508
#include <MX1508.h>                     
// Create motor object for each wheel  (Motor 1-  12,27   Motor 2- 33,15)
MX1508 motorA(12, 27, 0, 1);       //  Pin1, Pin2, PWMChannel 1, PWMChannel 2   (16 Channels availible 0-16)
MX1508 motorB(33, 15, 2, 3);
uint8_t speedL = 210;               // Set default speed
uint8_t speedR = 210;               // Set default speed

// FreeRTOS Tasks and Queue Declarations
QueueHandle_t commandQ;   // Queue for commands to be executed
//TaskHandle_t addCommand;  // Task handler function to add items to the queue
TaskHandle_t pcommandQ;  // Task handler function to remove items from the queue
TaskHandle_t distanceH; // Task handler for distance sensor readings



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
        motorA.motorGo(speedR);
        motorB.motorGo(speedL);
      } else if (commandData == 2) {
        motorA.motorRev(speedR);
        motorB.motorRev(speedL);
        //reverseRob();
      } else if (commandData == 3) {
        motorA.motorRev(speedR);
        motorB.motorGo(speedL);
        //turnRight();
      } else if (commandData == 4) {
        motorA.motorGo(speedR);
        motorB.motorRev(speedL);
        //turnLeft();
      } else if (commandData == 5) {
        vTaskSuspend(distanceH);                            // Suspend Autonomous mode
        motorA.stopMotor();
        motorB.stopMotor();
        //Serial.println("Auto Mode Suspended");
      } else if (commandData == 6) {
        vTaskResume(distanceH);                             // Start Autonomous mode
        motorA.motorGo(speedR);                             // Auto mode hangs after restarting when suspended during moving forward.  This fixes that, as the collision detection will stop immediately if too close when resuming.
        motorB.motorGo(speedL);
        //Serial.println("Auto Mode Started");
      } else if (commandData == 7) {
        //faster();
        speedR = 240;
        speedL = 240;
      } else if (commandData == 8) {
        //slower();
        speedR = 200;
        speedL = 200;
      
      } else
      Serial.print(commandData);
      Serial.print("|");
    }
  }
}

// Function used for measuring distance and acting as a basic autonomous mode.   
void distanceSensorfunc(void *pvParameters) {
  for(;;) {		// Run forever unless killed by task handle
    readingDistance = sensor.getRawDistance();
    if ((readingDistance <= 20) && (readingDistance >= 0 )) {         // Shutdown motors if less than 20mm.   Need to update with better overall logic-  this prevents backing up when something is in front too close
      motorA.stopMotor(); 
      motorB.stopMotor();
      motorA.motorRev(speedR);
      motorB.motorGo(speedL);
      vTaskDelay(600);
      motorA.stopMotor();
      motorB.stopMotor();
    }
    vTaskDelay(200);              // This seems to work for now.  Too fast might be affecting stability/kernel panic.  Check freq on PWM channel next.
    motorA.motorGo(speedR);
    motorB.motorGo(speedL);
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

  //// Create tasks for operational logic  
  
  // Motor Command Task 
  xTaskCreatePinnedToCore(
      pcommandQfunc, // Function to implement the task 
      "pcommandQ", // Name of the task 
      10000,  // Stack size in words 
      NULL,  // Task input parameter 
      2,  // Priority of the task                   // Don't run at 0 and loop() run's at 1.
      &pcommandQ,  // Task handle. 
      1); // Core where the task should run 
  
  // Distance Sensor Reading Task / Autonomous Mode
  xTaskCreatePinnedToCore(
      distanceSensorfunc, // Function to implement the task 
      "distanceSensor", // Name of the task 
      10000,  // Stack size in words 
      NULL,  // Task input parameter 
      4,  // Priority of the task                   // Higher than other tasks right now to allow priority for collision detection.
      &distanceH,  // Task handle. 
      1); // Core where the task should run 
  vTaskSuspend(distanceH);                         // Suspend task right after creation to allow for remote control by default
  

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
      //packet.printf("Header Recieved: %c", header[0]);
      packet.println(readingDistance);                      // Send any invalid header and the current distance reading is returned
      
    });
  }
  
}



void loop() {
ArduinoOTA.handle();    // TODO:  Move to own task and kill loop from running
vTaskDelay(1);          //  Removed delay() since we're using tasks and want to let loop() yield

}
