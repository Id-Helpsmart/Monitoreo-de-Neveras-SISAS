#ifndef DS18B20HS_H
#define DS18B20HS_H

#include <Arduino.h>
#include <map>
#include <iostream>
#include "DS18B20.h"

#define ONE_WIRE_BUS 21

extern bool trouble;
extern String message_trouble;

struct DS18B20Data
{
    char time[22] = "*";
    char temperature[9] = "*";
};

struct DS18B20Saved
{
    DS18B20Data _DS18B20Data;
    char energy[2] = "*";
};

struct keyValues
{
    enum Type
    {
        TEMPERATURE,
        TYPE_COUNT // Count of elements in the enum
    };

    Type type;
};

// Estructura para almacenar los límites de alarma
struct AlarmLimits
{
    int min;
    int max;
};
extern std::map<keyValues::Type, AlarmLimits> alarmLimits;
extern const char *keyValueToString(keyValues::Type keyValue);

class DS18B20HS
{
private:
public:
    DS18B20HS();
    ~DS18B20HS();
    void getData(DS18B20Data &);
};

#endif
