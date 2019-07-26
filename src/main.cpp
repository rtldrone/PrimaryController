#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>

extern "C" {
#include <bldc_interface_uart.h>
}

#define WS_WATCHDOG_TIMEOUT 2000

void send_packet(unsigned char *data, unsigned int len) {
    Serial1.write(data, len);
}

static void update_bldc_timer(void *parameter) {
    TickType_t xLastWakeTime;
    const TickType_t frequency = 1;
    xLastWakeTime = xTaskGetTickCount();

    //Run the timer every millisecond
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, frequency);
        bldc_interface_uart_run_timer();
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    Wire.setClock(100000);
    Wire.begin();
    SPIFFS.begin();

    bldc_interface_uart_init(send_packet);
    xTaskCreate(update_bldc_timer, "UpdateBldcTimer", 10000, nullptr, 1, nullptr);
}

void loop() {
    while (Serial1.available()) {
        unsigned char byteIn = Serial1.read();
        bldc_interface_uart_process_byte(byteIn);
    }

    delay(10);
}