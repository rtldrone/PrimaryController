//
// Created by cameronearle on 8/6/2019.
//

#ifndef PRIMARYCONTROLLER_VELOCITYPROFILER_H
#define PRIMARYCONTROLLER_VELOCITYPROFILER_H

#include <FreeRTOS.h>

class VelocityProfiler {
public:
    static void setVelocityTarget(float velocity);
    static float update();
    static void reset();
private:
    static float targetVelocity;
    static float currentVelocity;
    static SemaphoreHandle_t lock;
};

#endif //PRIMARYCONTROLLER_VELOCITYPROFILER_H
