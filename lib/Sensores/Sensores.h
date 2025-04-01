#ifndef Sensores_H
#define Sensores_H
#include "math.h"
#include <Arduino.h>

#include "DS18B20HS.h"
#include "funciones_spiffs.h"
#include "funciones_firebase.h"

#define PIN_4_INPUT_ENERGY 4    //  conector para medición pin GPIO4
#define PIN_36_BATTERY_LEVEL 36 // este pin aparece indicado en el esquema de ESP32 como SEN_VP

class Sensores
{
private:
    char notification_message[150] = "*";
    char voltaje_char[6];
    char name_card[50];
    char on[3] = "ON";
    char off[4] = "OFF";

    const float threshold = 10.0; // maximum allowed change
    const float battery_max = 3.3; // maximum voltage of battery
    const float battery_min = 2.6; // minimum voltage of battery before shutdown
    const float voltage_min = 2.8;


    float values[6];
    float mt1 = 0.0;
    float mt2 = 0.0;
    float battery_level = 0;
    float battery_voltage = 0;
    float voltage_percent = 0.0;
    
    bool previous_state_energy = false;
    bool state_energy = false;
    bool b_alarm = 1;
    
public:
    void Setup_Sen();
    void Update_Notify();
    void readData(DS18B20Data &envData);
    int Input_Energy(bool *p_emergency);
    float Battery_Level(bool *p_emergency);
};

#endif