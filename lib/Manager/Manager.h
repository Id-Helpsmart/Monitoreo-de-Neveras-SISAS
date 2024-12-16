#ifndef Manager_H
#define Manager_H
#include <Arduino.h>
#include <WiFi.h>

class Manager
{
private:

public:
    String id_lora();
    String id();
    String NetworkName();
};

#endif