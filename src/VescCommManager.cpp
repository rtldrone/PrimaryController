//
// Created by cameronearle on 7/26/2019.
//

#include <VelocityProfiler.h>
#include "VescCommManager.h"
#include "Constants.h"
#include "HMIWebserver.h"

extern "C" {
#include <bldc_interface_uart.h>
#include <bldc_interface.h>
}

//Static member initialization
Stream *VescCommManager::serial;
mc_values *VescCommManager::values = new mc_values; //Initialize a blank object to prevent unauthorized access
VescCommManager::VESCData VescCommManager::currentData{};
SemaphoreHandle_t VescCommManager::valuesLock = xSemaphoreCreateMutex();
SemaphoreHandle_t VescCommManager::commandLock = xSemaphoreCreateMutex();
float VescCommManager::currentDutyCycle = 0.0f;

void VescCommManager::sendSerialData(unsigned char *data, unsigned int length) {
    serial->write(data, length);
}

void VescCommManager::updateTimer(void *parameter) {
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

void VescCommManager::write(void *parameter) {
    TickType_t lastWakeTime;
    const TickType_t frequency = VESC_WRITE_RATE_MS;
    lastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, frequency);

        uint32_t lastValidHMIRecvTime = HMIWebserver::getLastValidRecvTime();
        if (lastWakeTime - lastValidHMIRecvTime >= HMI_WATCHDOG_TIMEOUT_MS) {
            VelocityProfiler::setVelocityTarget(0.0f); //Stop the vehicle if we time out the HMI
        }

        float velocity = VelocityProfiler::update();
        setPercentOut(velocity); //TODO use correct setter and units

        xSemaphoreTake(commandLock, portMAX_DELAY);
        bldc_interface_set_duty_cycle(currentDutyCycle);
        xSemaphoreGive(commandLock);
    }
}

void VescCommManager::onValues(mc_values *newValues) {
    xSemaphoreTake(valuesLock, portMAX_DELAY);
    values = newValues;
    currentData.inputVoltage = values->v_in;
    currentData.currentDraw = values->current_in;
    currentData.motorRpm = values->rpm;
    xSemaphoreGive(valuesLock);
}

void VescCommManager::begin(Stream *_serial) {
    serial = _serial;
    bldc_interface_uart_init(sendSerialData); //Register the callback function to send data out

    bldc_interface_set_rx_value_func(onValues); //Function to be called when mc_values are received

    xTaskCreatePinnedToCore( //Start the timer update task
            updateTimer,
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
    xTaskCreatePinnedToCore(
            write,
            "VescWrite",
            VESC_COMM_MANAGER_RT_STACK_DEPTH,
            nullptr,
            VESC_COMM_MANAGER_RT_PRIO,
            nullptr,
            1
    );
}

void VescCommManager::stop() {
    setPercentOut(0.0f); //Stop the motor
}

void VescCommManager::setPercentOut(float percentOut) {
    xSemaphoreTake(commandLock, portMAX_DELAY);
    currentDutyCycle = percentOut;
    xSemaphoreGive(commandLock);
}

float VescCommManager::getPercentOut() {
    xSemaphoreTake(commandLock, portMAX_DELAY);
    float toReturn = currentDutyCycle;
    xSemaphoreGive(commandLock);
    return toReturn;
}

VescCommManager::VESCData VescCommManager::getData() {
    VESCData data{};
    xSemaphoreTake(valuesLock, portMAX_DELAY);
    data.inputVoltage = currentData.inputVoltage;
    data.currentDraw = currentData.currentDraw;
    data.motorRpm = currentData.motorRpm;
    xSemaphoreGive(valuesLock);
    return data;
}