//
// Created by cameronearle on 8/6/2019.
//

#include "VelocityProfiler.h"
#include "Constants.h"

SemaphoreHandle_t VelocityProfiler::lock = xSemaphoreCreateMutex();
float VelocityProfiler::currentVelocity = 0.0f;
float VelocityProfiler::targetVelocity = 0.0f;

void VelocityProfiler::setVelocityTarget(float velocity) {
    xSemaphoreTake(lock, portMAX_DELAY);
    targetVelocity = velocity;
    xSemaphoreGive(lock);
}

void VelocityProfiler::reset() {
    xSemaphoreTake(lock, portMAX_DELAY);
    targetVelocity = 0.0f;
    currentVelocity = 0.0f;
    xSemaphoreGive(lock);
}

float VelocityProfiler::update() {
    float dv = MOTOR_ACCELERATION_PER_SECOND / (1000.0f / VESC_WRITE_RATE_MS);
    xSemaphoreTake(lock, portMAX_DELAY);
    if (targetVelocity > currentVelocity) {
        currentVelocity += dv;
        if (currentVelocity > targetVelocity) {
            //This add put us over, so set them equal
            currentVelocity = targetVelocity;
        }
    } else if (targetVelocity < currentVelocity) {
        currentVelocity -= dv;
        if (currentVelocity < targetVelocity) {
            //This subtract put us under, so set them equal
            currentVelocity = targetVelocity;
        }
    }
    xSemaphoreGive(lock);
    return currentVelocity;
}

float VelocityProfiler::getCurrentVelocity() {
    xSemaphoreTake(lock, portMAX_DELAY);
    float value = currentVelocity;
    xSemaphoreGive(lock);
    return value;
}