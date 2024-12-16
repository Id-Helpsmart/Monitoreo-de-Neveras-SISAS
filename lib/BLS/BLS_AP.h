#ifndef BLS_AP_H
#define BLS_AP_H

#include <Arduino.h> // Para poder usar las funciones básicas de Arduino
#include "Manager.h"
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
// #include <procesar_tramas.h>

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