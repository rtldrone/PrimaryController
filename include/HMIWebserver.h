//
// Created by cameronearle on 8/5/2019.
//

#ifndef PRIMARYCONTROLLER_HMIWEBSERVER_H
#define PRIMARYCONTROLLER_HMIWEBSERVER_H

#include <ESPAsyncWebServer.h>

#define HMI_INCOMING_CMD_UPDATE 'U'
#define HMI_INCOMING_CMD_STOP 'X'
#define HMI_INCOMING_CMD_SET_SPEED 'V'

#define HMI_OUTGOING_RESPONSE_SIZE \
  sizeof(float) /* Battery voltage */ \
+ sizeof(uint8_t) /* Battery state*/ \
+ sizeof(float) /* Current draw */ \
+ sizeof(float) /* Speed */ \
+ sizeof(float) /* Target speed */ \
+ sizeof(uint32_t) /* Fault code */

class HMIWebserver {
public:
    static void begin();

    /**
     * Returns the last time that a WebSocket packet was received from a client
     * @return The last time that a WebSocket packet was received from a client
     */
    static unsigned long getLastValidRecvTime();

    struct HMIIncomingPacket {
        enum IncomingCommand {
            UNKNOWN,
            UPDATE,
            STOP,
            SET_SPEED
        };

        bool valid = false;
        IncomingCommand command = UNKNOWN;
        float speedSetpoint = 0.0f;
    };
private:
    static AsyncWebServer webserver;
    static AsyncWebSocket websocket;

    static unsigned long lastValidRecvTime;

    static uint8_t *sendBuffer;

    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static HMIIncomingPacket parseIncomingPacket(const uint8_t *data, size_t len);

    static uint8_t calculateBatteryState(float voltage);

    static SemaphoreHandle_t lastValidRecvTimeMutex;
};

#endif //PRIMARYCONTROLLER_HMIWEBSERVER_H
