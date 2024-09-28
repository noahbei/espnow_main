// main esp
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

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

