//
// Created by cameronearle on 8/5/2019.
//

#ifndef PRIMARYCONTROLLER_FAULTMANAGER_H
#define PRIMARYCONTROLLER_FAULTMANAGER_H

#include <stdint.h>
#include <freertos/semphr.h>

class FaultManager {
public:
    enum Faults {
        VESC_COMM_FAULT,
        TEST_FAULT
    };

    static void registerFault(Faults fault);
    static void clearFault(Faults fault);

    static uint32_t getFaultCode();
private:
    static uint32_t faultCode;
    static SemaphoreHandle_t lock;
};

#endif //PRIMARYCONTROLLER_FAULTMANAGER_H
