#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <mutex>

#define WS_WATCHDOG_TIMEOUT 2000

const IPAddress apIP(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);
const char *ssid = "rtldrone";

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

std::mutex ws_recv_lock;
unsigned long ws_last_recvd = 0;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  ws_recv_lock.lock();
  if (type == WS_EVT_CONNECT) {
    Serial.println("WS Client Connected");
  }
  if (type == WS_EVT_DATA) {
    ws_last_recvd = millis();
    if (len == 1 && data[0] == 'U') {
      //Update command, log it
      Serial.println("Got Update");
      //Respond
      client->text(String(ws_last_recvd));
    }
  }
  ws_recv_lock.unlock();
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP(ssid);
  webserver.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);
  webserver.begin();
}

void loop() {
  ws_recv_lock.lock();
  unsigned long current_time = millis();
  unsigned long dt = current_time - ws_last_recvd;
  if (dt > WS_WATCHDOG_TIMEOUT) {
    Serial.println("WS TIMEOUT");
  }
  ws_recv_lock.unlock();
  delay(10);
}