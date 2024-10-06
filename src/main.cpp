// main esp
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
//#include "main.h"
void sendData(uint8_t *address, bool influx, bool outflux);
void handleRootRequest(AsyncWebServerRequest *request);
void handleWaterPost(AsyncWebServerRequest *request);
void handleOutfluxPost(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

uint8_t module1Address[] = {0xFC, 0xE8, 0xC0, 0x7C, 0xCC, 0x10};
uint8_t module2Address[] = {0xAC, 0x15, 0x18, 0xC0, 0x02, 0x8C};

bool myInfluxOpen1 = false;
bool myOutfluxOpen1 = false;
bool myInfluxOpen2 = false;
bool myOutfluxOpen2 = false;

typedef struct struct_module_message {
  // module number
  uint8_t module;

  // flow rate data
  float flowRate;
  unsigned int flowMilliLitres;
  unsigned long totalMilliLitres;

  // system status
  bool influxOpen;
  bool outfluxOpen;
} module_message;
module_message myModuleData;

typedef struct struct_main_message {
  // system status
  bool influxOpen;
  bool outfluxOpen;
} main_message;
main_message myMainData;

// Peer info
esp_now_peer_info_t peerInfo;
 
// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback function executed when data is received
// update local variables with the flow rate and stuff so we can do stuff with it
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myModuleData, incomingData, sizeof(myModuleData));
  Serial.println("ON RECEIVE");
  // Print the MAC address of the sender
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();

  Serial.print("Data received: ");
  Serial.println(len);
  Serial.print("module number: ");
  Serial.println(myModuleData.module);
  Serial.print("Flow rate: ");
  Serial.println(myModuleData.flowRate);
  Serial.print("volume: ");
  Serial.println(myModuleData.totalMilliLitres);
  Serial.print("influx: ");
  Serial.println(myModuleData.influxOpen);
  Serial.print("influx: ");
  Serial.println(myModuleData.outfluxOpen);
  Serial.println();

  // Create a JSON object with the data
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["module"] = myModuleData.module;
  jsonDoc["flowRate"] = myModuleData.flowRate;
  jsonDoc["totalMilliLitres"] = myModuleData.totalMilliLitres;
  jsonDoc["influxOpen"] = myModuleData.influxOpen;
  jsonDoc["outfluxOpen"] = myModuleData.outfluxOpen;

  // Serialize JSON to string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Send the data to all WebSocket clients
  ws.textAll(jsonString);
}
const char *ssid = "ESP32_AP";
const char *password = "12345678";

const int mosPin = 4;

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  
  // Set up pump pinss
  pinMode(mosPin, OUTPUT);
  digitalWrite(mosPin, LOW); // Start with pump off

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);

  // Print the IP address
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // Register peer for module 1
  memcpy(peerInfo.peer_addr, module1Address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer 1");
      return;
  }

  // Register peer for module 2
  memcpy(peerInfo.peer_addr, module2Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer 2");
      return;
  }

    // Handle WebSocket events
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Handle HTTP requests
    server.on("/", HTTP_GET, handleRootRequest);
    server.on("/water-module-2", HTTP_GET, handleWaterGet2);
    server.on("/outflux-module-2", HTTP_GET, handleOutfluxGet2); // for demo and testing
    server.on("/water-module-1", HTTP_GET, handleWaterGet1);
    server.on("/outflux-module-1", HTTP_GET, handleOutfluxGet1); // for demo and testing

    // Define the route to turn the pump on
    server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(mosPin, HIGH);
      request->send(200, "text/plain", "Pump turned on");
    });

    // Define the route to turn the pump off
    server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(mosPin, LOW);
      request->send(200, "text/plain", "Pump turned off");
    });

    server.on("/m1i", HTTP_GET, [](AsyncWebServerRequest *request) {
      sendData(module1Address, 1, 0);
      request->send(200, "text/plain", "module 2 influx opened");
    });

    server.on("/m1o", HTTP_GET, [](AsyncWebServerRequest *request) {
      sendData(module1Address, 0, 1);
      request->send(200, "text/plain", "module 2 outflux opened");
    });
    // Start server
    server.begin();
}

 void sendData(uint8_t *address, bool influx, bool outflux) {
  myMainData.influxOpen = influx;
  myMainData.outfluxOpen = outflux;
  esp_now_send(address, (uint8_t *) &myMainData, sizeof(myMainData));
}

void loop() {
  // Check if 500 milliseconds have passed since actionTriggered1
  if (millis() - actionTriggered1 >= interval && actionTriggered1 != 0) {
    digitalWrite(mosPin, LOW);
    actionTriggered1 = 0; // Reset the trigger time to avoid repeated actions
    Serial.println("Water module 1 action completed, MOSFET set to LOW");
    myInfluxOpen1 = 0;
  }

  // Check if 500 milliseconds have passed since actionTriggered2
  if (millis() - actionTriggered2 >= interval && actionTriggered2 != 0) {
    digitalWrite(mosPin, LOW);
    actionTriggered2 = 0; // Reset the trigger time to avoid repeated actions
    Serial.println("Water module 2 action completed, MOSFET set to LOW");
    myInfluxOpen2 = 0;
  }

  // Check if 6 seconds have passed since outfluxTriggered1
  if (millis() - outfluxTriggered1 >= outfluxInterval && outfluxTriggered1 != 0) {
    sendData(module1Address, myInfluxOpen1, 0);
    outfluxTriggered1 = 0; // Reset the trigger time to avoid repeated actions
    Serial.println("Outflux module 1 action completed, valve closed");
    myOutfluxOpen1 = 0;
  }

  // Check if 6 seconds have passed since outfluxTriggered2
  if (millis() - outfluxTriggered2 >= outfluxInterval && outfluxTriggered2 != 0) {
    sendData(module2Address, myInfluxOpen2, 0);
    outfluxTriggered2 = 0; // Reset the trigger time to avoid repeated actions
    Serial.println("Outflux module 2 action completed, valve closed");
    myOutfluxOpen2 = 0;
  }
}

void handleRootRequest(AsyncWebServerRequest *request) {
  String responseHtml = "<h1>Hello, World!</h1>";
  request->send(200, "text/html", responseHtml);
}

unsigned long actionTriggered1;
unsigned long actionTriggered2;
unsigned long outfluxTriggered1;
unsigned long outfluxTriggered2;
const unsigned long interval = 500; // 500 milliseconds
const unsigned long outfluxInterval = 6000; // 6000 milliseconds (6 seconds)

void handleWaterGet1(AsyncWebServerRequest *request) {
  myInfluxOpen1 = 1;
  sendData(module1Address, myInfluxOpen1, myOutfluxOpen1);
  digitalWrite(mosPin, HIGH);
  actionTriggered1 = millis();
  request->send(200, "text/plain", "Water module 1 action triggered");
}

void handleOutfluxGet1(AsyncWebServerRequest *request) {
  myOutfluxOpen1 = 1;
  sendData(module1Address, myInfluxOpen1, myOutfluxOpen1);
  outfluxTriggered1 = millis();
  request->send(200, "text/plain", "Outflux module 1 action triggered");
}

void handleWaterGet2(AsyncWebServerRequest *request) {
  myInfluxOpen2 = 1;
  sendData(module2Address, myInfluxOpen2, myOutfluxOpen2);
  digitalWrite(mosPin, HIGH);
  actionTriggered2 = millis();
  request->send(200, "text/plain", "Water module 2 action triggered");
}

void handleOutfluxGet2(AsyncWebServerRequest *request) {
  myOutfluxOpen2 = 1;
  sendData(module2Address, myInfluxOpen2, myOutfluxOpen2);
  outfluxTriggered2 = millis();
  request->send(200, "text/plain", "Outflux module 2 action triggered");
}
// WebSocket event handler
// void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
//                AwsEventType type, void *arg, uint8_t *data, size_t len) {
//     if (type == WS_EVT_CONNECT) {
//         Serial.printf("Client connected: %u\n", client->id());
//     } else if (type == WS_EVT_DISCONNECT) {
//         Serial.printf("Client disconnected: %u\n", client->id());
//     } else if (type == WS_EVT_DATA) {
//         // Handle incoming data from the web dashboard
//         String message = "";
//         for (size_t i = 0; i < len; i++) {
//             message += (char)data[i];
//         }
//         Serial.printf("Received message: %s\n", message.c_str());
//     }
// }
// simplified version
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}