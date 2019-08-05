//
// Created by cameronearle on 8/5/2019.
//

/**
 * This file defines a list of constants and other configuration parameters that can be adjusted.
 * The file is divided into sections related to each sub-module of the code
 */

#ifndef PRIMARYCONTROLLER_CONFIG_H
#define PRIMARYCONTROLLER_CONFIG_H

/**
 * Debug configuration
 */

//Comment out this line to disable debugging
#define DO_DEBUG

#ifdef DO_DEBUG
#define DEBUG_LOG(x) Serial.println(x)
#else
#define DEBUG_LOG(x)
#endif

/**
 * Wi-Fi and webserver configuration
 */

//Address of the ESP-32 on the created Wi-Fi network
#define SOFT_AP_IP 192, 168, 4, 1

//Subnet mask on the created Wi-Fi network
#define SOFT_AP_SUBNET_MASK 255, 255, 255, 0

//SSID of the created Wi-Fi network
#define SOFT_AP_SSID "rtldrone"

//Number of milliseconds to wait until timing out when a WebSocket connection is lost
#define WS_WATCHDOG_TIMEOUT_MS 2000

/**
 * Peripheral config
 */

//Which serial interface to use when communicating with the VESC
#define VESC_SERIAL_INTERFACE Serial2

//Baudrate to use when communicating with the VESC
#define VESC_SERIAL_BAUDRATE 115200

#define WIRE_CLOCK 100000

/**
 * Timing configuration
 */

//How frequently to request values from the VESC
#define VESC_READ_RATE_MS 500

/**
 * Units and conversions
 */

//Macro to convert from motor shaft rpm directly to linear MPH
#define MOTOR_RPM_TO_MPH(rpm) rpm //TODO add correct conversion

#endif //PRIMARYCONTROLLER_CONFIG_H
