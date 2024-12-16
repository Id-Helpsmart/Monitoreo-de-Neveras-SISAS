#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <Arduino.h>
#include "ESP32Time.h"
#include <HTTPClient.h>

#define API_TIME "https://siap-api.helpmedica.com/api/time"

class Timestamp
{
public:
    void synchronizeTimestamp();
    String getTime();

private:
    ESP32Time rtc;
};

#endif