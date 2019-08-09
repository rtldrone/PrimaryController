//
// Created by cameronearle on 8/5/2019.
//

#include "Constants.h"
#include "HMIWebserver.h"
#include "VescCommManager.h"
#include "FaultManager.h"
#include <SPIFFS.h>
#include "VelocityProfiler.h"

extern "C" {
#include <buffer.h> //Make use of VESC buffer library since it's simple
}

//Static member initialization
AsyncWebServer HMIWebserver::webserver(80);
AsyncWebSocket HMIWebserver::websocket("/ws");
unsigned long HMIWebserver::lastValidRecvTime = 0UL;
uint8_t *HMIWebserver::sendBuffer = (uint8_t*) malloc(HMI_OUTGOING_RESPONSE_SIZE);
SemaphoreHandle_t HMIWebserver::lastValidRecvTimeMutex = xSemaphoreCreateMutex();

uint8_t HMIWebserver::calculateBatteryState(float voltage) {
    if (voltage >= BATTERY_OK_THRESHOLD_VOLTS) return 0;
    if (voltage >= BATTERY_WARN_THRESHOLD_VOLTS) return 1;
    return 2;
}

void HMIWebserver::begin() {
    webserver.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    websocket.onEvent(onWsEvent);
    webserver.addHandler(&websocket);
    webserver.begin();
}

void HMIWebserver::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    //Note - All incoming data is LITTLE ENDIAN, all outgoing data is BIG ENDIAN
    //This is because the ESP-32 is configured for little endian, so memcpy on incoming data is little endian
    //But our buffer library is big endian
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
            xSemaphoreTake(lastValidRecvTimeMutex, portMAX_DELAY);
            lastValidRecvTime = time;
            xSemaphoreGive(lastValidRecvTimeMutex);
            if (packet.command == HMIIncomingPacket::UPDATE) {
                DEBUG_LOG("Received update packet");
                auto vescData = VescCommManager::getData();
                auto targetSpeed = VelocityProfiler::getCurrentVelocity();
                auto faultCode = FaultManager::getFaultCode();
                int32_t i = 0;
                
                buffer_append_float32_auto(sendBuffer, vescData.inputVoltage, &i); //Battery voltage (4 bytes)
                buffer_append_uint8(sendBuffer, calculateBatteryState(vescData.inputVoltage), &i); //Battery State (1 byte)
                buffer_append_float32_auto(sendBuffer, vescData.currentDraw, &i); //Current draw (4 bytes)
                buffer_append_float32_auto(sendBuffer, vescData.motorRpm, &i); //Speed (4 bytes)
                buffer_append_float32_auto(sendBuffer, targetSpeed, &i); //Target speed (4 bytes)
                buffer_append_uint32(sendBuffer, faultCode, &i); //Fault code (4 bytes)

                client->binary(sendBuffer, HMI_OUTGOING_RESPONSE_SIZE);
            } else if (packet.command == HMIIncomingPacket::STOP) {
                DEBUG_LOG("Received stop packet");
                VelocityProfiler::setVelocityTarget(0.0f);
            } else if (packet.command == HMIIncomingPacket::SET_SPEED) {
                DEBUG_LOG("Received set speed packet");
                VelocityProfiler::setVelocityTarget(packet.speedSetpoint / 50.0f); //TODO use correct value
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
        uint8_t cmd = data[0];
        if (cmd == HMI_INCOMING_CMD_UPDATE) {
            packet.command = HMIIncomingPacket::UPDATE;
            packet.valid = true;
        } else if (cmd == HMI_INCOMING_CMD_STOP) {
            packet.command = HMIIncomingPacket::STOP;
            packet.valid = true;
        } else if (cmd == HMI_INCOMING_CMD_SET_SPEED) {
            packet.command = HMIIncomingPacket::SET_SPEED;
            if (len == 5) { //1 byte command identifier + 4 bytes of floating point data
                packet.valid = true;
                memcpy(&packet.speedSetpoint, data + 1, sizeof(packet.speedSetpoint));
            } else {
                packet.valid = false;
            }
        } else {
            packet.valid = false;
            packet.command = HMIIncomingPacket::UNKNOWN;
        }
    } else {
        //A packet with zero length is always invalid
        packet.valid = false;
    }
    return packet;
}

unsigned long HMIWebserver::getLastValidRecvTime() {
    xSemaphoreTake(lastValidRecvTimeMutex, portMAX_DELAY);
    uint32_t time = lastValidRecvTime;
    xSemaphoreGive(lastValidRecvTimeMutex);
    return time;
}