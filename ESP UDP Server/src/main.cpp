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

// Servo Library
#include <ESP32Servo.h>
Servo myservo;
int servoPin = 21;


// Sharp IR Sensor Library from https://github.com/NuwanJ/ESP32SharpIR.git
#include <ESP32SharpIR.h>
ESP32SharpIR sensor( ESP32SharpIR::GP2Y0A21YK0F, 32);     // Must use pin on ADC1 else will crash when Wifi is used
uint8_t readingDistance;       

// MX1508 Library from:  https://github.com/ElectroMagus/ESP32MX1508.git
#include <ESP32MX1508.h>                     
// Create motor object for each wheel  (Motor 1-  12,27   Motor 2- 33,15)
MX1508 motorA(12, 27, 11, 12);       //  Pin1, Pin2, PWMChannel 1, PWMChannel 2   (16 Channels availible 0-15)
MX1508 motorB(33, 15, 13, 14);
uint8_t speedL = 210;               // Set default speed
uint8_t speedR = 210;               // Set default speed

// FreeRTOS Task and Queue Handle Declarations
QueueHandle_t CommQ;   // Queue for commands
TaskHandle_t procCommQ;  // Handler for task to process commands
TaskHandle_t distanceH; // Handler for distance sensor readings



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
        vTaskSuspend(distanceH);
        // Suspend Autonomous mode  -- Stop motor function when suspending this task                          
        motorA.stopMotor();
        motorB.stopMotor();
      } else if (commandData == 6) {
        // Start Autonomous mode  -- Start motor function when suspending this task or motors will not restart until distance sensor is triggered                         
        vTaskResume(distanceH);   
        motorA.motorGo(speedR);                             
        motorB.motorGo(speedL); 
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
        myservo.write(180);
        
      } else
      // This section runs regardless of the commandData sent
      Serial.print(commandData);      
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

  // Servo Setup
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 1000, 2000); 

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
  

  // To-Do:  Move UDP Server to Task

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
        xQueueSend(CommQ, &select, portMAX_DELAY);   // Add's the int after the F header to the FreeRTOS Command Queue for processing/scheduling to free up the UDP connection 
        packet.printf("OK - %i", select);
      } else

      // All other packets, just return the header to the client for diagnostics
      //packet.printf("Header Recieved: %c", header[0]);
      packet.println(readingDistance);                      // Send any invalid header and the current distance reading is returned
      
    });
  }
  
}



void loop() {
ArduinoOTA.handle();    // To-Do:  Move to own task and kill loop from running
vTaskDelay(1);          //  Removed delay() since we're using tasks and want to let loop() yield

}
