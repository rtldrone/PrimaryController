//
// Created by cameronearle on 7/26/2019.
//

#include "VescCommManager.h"
#include "Constants.h"

extern "C" {
#include <bldc_interface_uart.h>
#include <bldc_interface.h>
}

//Static member initialization
Stream *VescCommManager::serial;
mc_values *VescCommManager::values = new mc_values; //Initialize a blank object to prevent unauthorized access
VescCommManager::VESCData VescCommManager::currentData{};
SemaphoreHandle_t VescCommManager::valuesLock;

void VescCommManager::sendSerialData(unsigned char *data, unsigned int length) {
    serial->write(data, length);
}

void VescCommManager::update(void *parameter) {
    TickType_t lastWakeTime;
    const TickType_t frequency = VESC_COMM_MANAGER_RT_FREQUENCY;
    lastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, frequency);

        for (int i = 0; serial->available() && i < VESC_COMM_MANAGER_MAX_BUFFER_READ_PER_CYCLE; i++) {
            unsigned char byteIn = serial->read();
            bldc_interface_uart_process_byte(byteIn); //Send the byte to the VESC state machine
        }

        bldc_interface_uart_run_timer(); //Update the VESC state machine
    }
}

void VescCommManager::read(void *parameter) {
    TickType_t lastWakeTime;
    const TickType_t frequency = VESC_READ_RATE_MS;
    lastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, frequency);

        xSemaphoreTake(valuesLock, portMAX_DELAY);
        bldc_interface_get_values();
        xSemaphoreGive(valuesLock);
    }
}

void VescCommManager::onValues(mc_values *newValues) {
    xSemaphoreTake(valuesLock, portMAX_DELAY);
    values = newValues;
    currentData.inputVoltage = values->v_in;
    currentData.inputCurrentDraw = values->current_in;
    currentData.motorCurrentDraw = values->current_motor;
    currentData.motorRpm = values->rpm;
    xSemaphoreGive(valuesLock);
}

void VescCommManager::begin(Stream *_serial) {
    serial = _serial;
    bldc_interface_uart_init(sendSerialData); //Register the callback function to send data out

    bldc_interface_set_rx_value_func(onValues); //Function to be called when mc_values are received

    valuesLock = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore( //Start the timer update task
            update,
            "VescStateMachine",
            VESC_COMM_MANAGER_RT_STACK_DEPTH,
            nullptr,
            VESC_COMM_MANAGER_RT_PRIO,
            nullptr,
            1
    );
    xTaskCreatePinnedToCore( //Start the read task
            read,
            "VescRead",
            VESC_COMM_MANAGER_RT_STACK_DEPTH,
            nullptr,
            VESC_COMM_MANAGER_RT_PRIO,
            nullptr,
            1
    );
}

void VescCommManager::setPercentOut(float percentOut) {
    bldc_interface_set_duty_cycle(percentOut);
}

VescCommManager::VESCData VescCommManager::getData() {
    VESCData data{};
    xSemaphoreTake(valuesLock, portMAX_DELAY);
    data.inputVoltage = currentData.inputVoltage;
    data.inputCurrentDraw = currentData.inputCurrentDraw;
    data.motorCurrentDraw = currentData.motorCurrentDraw;
    data.motorRpm = currentData.motorRpm;
    xSemaphoreGive(valuesLock);
    return data;
}