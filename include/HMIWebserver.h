//
// Created by cameronearle on 8/5/2019.
//

#ifndef PRIMARYCONTROLLER_HMIWEBSERVER_H
#define PRIMARYCONTROLLER_HMIWEBSERVER_H

#include <ESPAsyncWebServer.h>

#define HMI_INCOMING_CMD_UPDATE 'U'

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
            UPDATE
        };

        bool valid = false;
        IncomingCommand command = UNKNOWN;
    };
private:
    static AsyncWebServer webserver;
    static AsyncWebSocket websocket;

    static const char *updateResponseFmt;

    static unsigned long lastValidRecvTime;

    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static HMIIncomingPacket parseIncomingPacket(const uint8_t *data, size_t len);
};

#endif //PRIMARYCONTROLLER_HMIWEBSERVER_H
