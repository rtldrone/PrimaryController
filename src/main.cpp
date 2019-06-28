#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define WS_WATCHDOG_TIMEOUT 2000

enum BatteryState {
  OK,
  WARNING,
  BAD
};

const IPAddress apIP(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);
const char *ssid = "rtldrone";
const char *oiResponseFmt = "S%.2f%i%.2f%.2fF%s"; //S[battery voltage][battery state][speed][speed setpoint]F[faults]

//Live data
double batteryVoltage = 12.00;
int batteryState = BatteryState::OK;
double currentSpeedMph = 5.00;
double speedSetpointMph = 10.00;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

unsigned long ws_last_recvd = 0;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WS Client Connected");
  }
  if (type == WS_EVT_DATA) {
    ws_last_recvd = millis();
    if (len == 1 && data[0] == 'U') {
      //Update command, log it
      Serial.println("Got Update");
      //Respond
      client->printf(oiResponseFmt, batteryVoltage, batteryState, currentSpeedMph, speedSetpointMph, "");
    }
  }
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
  unsigned long current_time = millis();
  unsigned long dt = current_time - ws_last_recvd;
  if (dt > WS_WATCHDOG_TIMEOUT) {
    Serial.println("WS TIMEOUT");
  }
  delay(10);
}