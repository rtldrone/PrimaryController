//
// Created by cameronearle on 8/5/2019.
//

#include <FreeRTOS.h>
#include "FaultManager.h"

SemaphoreHandle_t FaultManager::lock = xSemaphoreCreateMutex();
uint32_t FaultManager::faultCode = 0UL;

void FaultManager::registerFault(FaultManager::Faults fault) {
    auto ordinal = (uint32_t) fault;
    xSemaphoreTake(lock, portMAX_DELAY);
    faultCode |= 1UL << ordinal;
    xSemaphoreGive(lock);
}

void FaultManager::clearFault(FaultManager::Faults fault) {
    auto ordinal = (uint32_t) fault;
    xSemaphoreTake(lock, portMAX_DELAY);
    faultCode &= ~(1UL << ordinal);
    xSemaphoreGive(lock);
}

uint32_t FaultManager::getFaultCode() {
    xSemaphoreTake(lock, portMAX_DELAY);
    auto value = faultCode;
    xSemaphoreGive(lock);
    return value;
}