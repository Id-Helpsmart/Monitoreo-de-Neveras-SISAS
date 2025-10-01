#ifndef Wifi_Mqtt_H
#define Wifi_Mqtt_H

#include <Arduino.h> // Para poder usar las funciones básicas de Arduino
#include <WiFi.h>
// #include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Manager.h"
#include <DNSServer.h>
#include <WebServer.h>
#include <HTTPClient.h>

// credenciales para la red generada por el esp32
// const char *ssid = "ESP32-AP";
extern const char *password;

// Configuración para una IP fija y omitir el DHCP
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;

//**************************************
//*********** MQTT CONFIG **************
//**************************************
extern const char *mqtt_server;
extern const int mqtt_port;
extern const char *mqtt_user;
extern const char *mqtt_pass;

// extern long interval; // interval at which to send mqtt messages (milliseconds)

extern bool use_led;
extern bool server_status;

extern volatile bool emergency;
extern volatile bool completed_time;

// Variable to store the HTTP request
//extern String topic_pub;
//extern String topic_rtc;
extern String topic_sub;

extern WiFiClient espClient;
extern PubSubClient client_mqtt;
extern Manager _manager;

// Set web server port number to 81
extern WiFiServer server;

class Wifi_Mqtt
{
private:
    const char *mqtt_user = "device1.helpmedica";
    const char *mqtt_pass = "device1.helpmedica";

public:
    bool verify_internet();
    void conectar_wifi(char *p_nombre_wifi, char *p_clave_wifi);
    void reconnect(char *p_nombre_wifi, char *p_clave_wifi);
};

#endif