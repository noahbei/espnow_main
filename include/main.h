#include <ESPAsyncWebServer.h>
#include <esp_now.h>

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void sendData(uint8_t *address, bool influx, bool outflux);
void handleRootRequest(AsyncWebServerRequest *request);
void handleWaterPost(AsyncWebServerRequest *request);
void handleOutfluxPost(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void handleRootRequest(AsyncWebServerRequest *request);
void handleWaterGet1(AsyncWebServerRequest *request);
void handleOutfluxGet1(AsyncWebServerRequest *request);
void handleWaterGet2(AsyncWebServerRequest *request);
void handleOutfluxGet2(AsyncWebServerRequest *request);
