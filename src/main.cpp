// main esp
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

uint8_t module1Address[] = {0xFC, 0xE8, 0xC0, 0x7C, 0xCC, 0x10};
uint8_t module2Address[] = {0xAC, 0x15, 0x18, 0xC0, 0x02, 0x8C};

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
}
const char *ssid = "ESP32_AP";
// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  
  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

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

  WiFi.mode(WIFI_AP);
  // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Handle WebSocket events
    // ws.onEvent(onWsEvent);
    // server.addHandler(&ws);

    // Handle HTTP requests
    server.on("/", HTTP_GET, handleRootRequest);
    server.on("/water-module", HTTP_POST, handleWaterPost);
    server.on("/outflux-module", HTTP_POST, handleOutfluxPost); // for demo and testing

    // Start server
    server.begin();
}
 void sendData(uint8_t *address, bool influx, bool outflux) {
  myMainData.influxOpen = influx;
  myMainData.outfluxOpen = outflux;
  esp_now_send(address, (uint8_t *) &myMainData, sizeof(myMainData));
}
void loop() {
  sendData(module1Address, 1, 1);
  delay(2000);
  sendData(module2Address, 1, 0);
  delay(2000);
}

void handleRootRequest(AsyncWebServerRequest *request) {
    String responseHtml = "<h1>Hello, World!</h1>";
    request->send(200, "text/html", responseHtml);
}

// Function to handle POST requests to /water-module
// Web Dashboard will tell main esp32 what module to initiate watering
// have a blocking bool to make sure that only one module can be filled at a time (for simplicity)
void handleWaterPost(AsyncWebServerRequest *request) {
    // Check if the request has a body (if required)
    if (request->hasParam("data", true)) {
        String data = request->getParam("data", true)->value();
        Serial.printf("Water Module Data Received: %s\n", data.c_str());

        // TODO: Process the received data as needed
    }

    // Send a response back to the client
    request->send(200, "text/plain", "Water module data received");
}

// Function to handle POST requests to /outflux-module
// Web Dashboard will tell main esp32 what module to release water from
// This is for testing
void handleOutfluxPost(AsyncWebServerRequest *request) {
    // Check if the request has a body (if required)
    if (request->hasParam("data", true)) {
        String data = request->getParam("data", true)->value();
        Serial.printf("Outflux Module Data Received: %s\n", data.c_str());

        // TODO: Process the received data as needed
    }

    // Send a response back to the client
    request->send(200, "text/plain", "Outflux module data received");
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