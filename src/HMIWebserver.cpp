//
// Created by cameronearle on 8/5/2019.
//

#include "Constants.h"
#include "HMIWebserver.h"
#include "VescCommManager.h"
#include "FaultManager.h"
#include <SPIFFS.h>

//Static member initialization
AsyncWebServer HMIWebserver::webserver(80);
AsyncWebSocket HMIWebserver::websocket("/ws");
unsigned long HMIWebserver::lastValidRecvTime = 0UL;

//[battery voltage],[battery state],[speed],[speed setpoint]:[fault code]
const char *HMIWebserver::updateResponseFmt = "%.2f,%i,%.2f,%.2f:%lu";

void HMIWebserver::begin() {
    webserver.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    websocket.onEvent(onWsEvent);
    webserver.addHandler(&websocket);
    webserver.begin();
}

void HMIWebserver::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    unsigned long time = millis();

    if (type == WS_EVT_CONNECT) {
        DEBUG_LOG("New WS Client Connected");
    }
    if (type == WS_EVT_DISCONNECT) {
        DEBUG_LOG("WS Client Disconnected");
    }
    if (type == WS_EVT_DATA) {
        HMIIncomingPacket packet = parseIncomingPacket(data, len);
        if (packet.valid) {
            lastValidRecvTime = time;
            if (packet.command == HMIIncomingPacket::UPDATE) {
                DEBUG_LOG("Received update packet");
                auto vescData = VescCommManager::getData();
                auto faultCode = FaultManager::getFaultCode();
                client->printf(updateResponseFmt,
                               vescData.inputVoltage,
                               0.0f, //TODO add battery state
                               MOTOR_RPM_TO_MPH(vescData.motorRpm),
                               0.0f, //TODO add setpoint rpm
                               faultCode
                );
            } else {
                DEBUG_LOG("Unknown valid packet received?"); //This shouldn't happen
            }
        } else {
            DEBUG_LOG("Received invalid packet");
        }
    }
}

HMIWebserver::HMIIncomingPacket HMIWebserver::parseIncomingPacket(const uint8_t *data, size_t len) {
    HMIIncomingPacket packet{};
    if (len >= 1) {
        switch(data[0]) {
            case HMI_INCOMING_CMD_UPDATE:
                packet.valid = true;
                packet.command = HMIIncomingPacket::UPDATE;
                break;
            default:
                packet.valid = false;
                packet.command = HMIIncomingPacket::UNKNOWN;
                break;
        }
    } else {
        //A packet with zero length is always invalid
        packet.valid = false;
    }
    return packet;
}

unsigned long HMIWebserver::getLastValidRecvTime() {
    return lastValidRecvTime;
}