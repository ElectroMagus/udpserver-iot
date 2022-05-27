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
TaskHandle_t remCommand;  // Task handler function to remove items from the queue

void setup() {
  Serial.begin(115200);
  // OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  ArduinoOTA.setHostname("servo");                // mDNS hostname
  ArduinoOTA.begin();

  // FreeRTOS Queue Setup
  commandQ = xQueueCreate( 20, sizeof( int ) );		// args:  # of items, size in byte per item
  if(commandQ == NULL) {				// If unable to create the queue, output to serial
    Serial.println("Error creating the queue");
    }


   // Setup UDP Server to listen
  if(udp.listen(20001)) {
    // When a packet is recieved
    udp.onPacket([](AsyncUDPPacket packet) {
      // Copies the packet data from memory into a string for parsing.   
      // packet.data is unit8_t *, so possibly do a comparison directly vs the memcpy to speed up the transaction
      char* tmpStr = (char*) malloc(packet.length() + 1);	// Create char array of size+1 of packet data
      memcpy(tmpStr, packet.data(), packet.length());		// Copy packet data to array
      tmpStr[packet.length()] = '\0';				// Terminate data in array with a NULL character
      String header = String(tmpStr);				// Copy array to String for parsing
      free(tmpStr); 						// Free up that memory

      if (header[0] == 'F') {					// Using a leading F as a header for testing
        int select = header[1] - '0';     			// Converts the string to an integer during declaration to make the compiler happy
        xQueueSend(commandQ, &select, portMAX_DELAY);   	// Add's the int after the F header to the FreeRTOS Command Queue for processing/scheduling to free up the UDP connection 
        packet.printf("OK - %i", select);			// Send reply to client for diagnostics
      } else

      // All other packets, just return the header to the client for diagnostics
        packet.printf("Header Recieved: %c", header[0]);
    });
  }
}



void loop() {

// Processing the command queue every time loop() runs.  Will offload queues with FreeRTOS tasks in the future.
int commandData;   // variable used to process the next command on the command queue
for(int i = 0; i < 20; i++) {		// Update to match size of commandQ
  // Print all items currently in the command queue to the serial port
    xQueueReceive(commandQ, &commandData, portMAX_DELAY);
    Serial.print(commandData);
    Serial.print("|");
}

ArduinoOTA.handle();
delay(1);

}
