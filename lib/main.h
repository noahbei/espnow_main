#include <ESPAsyncWebServer.h>

void handleRootRequest(AsyncWebServerRequest *request);
void handleWaterPost(AsyncWebServerRequest *request);
void handleOutfluxPost(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);