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

// FreeRTOS Tasks and Queue Declarations
QueueHandle_t commandQ;   // Queue for commands to be executed
TaskHandle_t addCommand;  // Task handler function to add items to the queue
TaskHandle_t pcommandQ;  // Task handler function to remove items from the queue

// Function that runs when the commandQ task is invoked
void pcommandQfunc(void *pvParameters) {
  for(;;) {
    int commandData;   // variable used to process the next command on the command queue
    for(int i = 0; i<20; i++){
      // Print all items currently in the command queue to the serial port
      xQueueReceive(commandQ, &commandData, portMAX_DELAY);
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
  ArduinoOTA.setHostname("servo");                // mDNS hostname
  ArduinoOTA.begin();
  
  // Initialize Servo Board
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  

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

}
