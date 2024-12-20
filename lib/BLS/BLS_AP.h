#ifndef BLS_AP_H
#define BLS_AP_H

#include <Arduino.h> // Para poder usar las funciones básicas de Arduino
#include "Manager.h"
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CONFIG_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
//#define LED_CHARACTERISTIC_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

// extern bool use_led;
extern bool close_settings;
extern bool flag_callback_broker;
extern bool configuration_updated;

extern JsonDocument doc_config;

extern TickType_t led_flashing_lock_time;

class BLS_AP
{
private:
public:
    void close_settings();
    void Access_Point(bool *p_estado_servidor, void Idle_Reboot_AP());
    // void Access_Point(bool *p_estado_servidor, void Idle_Reboot_AP());
};

#endif