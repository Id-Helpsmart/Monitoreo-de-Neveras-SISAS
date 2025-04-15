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

// extern String topic_pub;
// extern String topic_rtc;
extern String topic_sub;

extern WiFiClient espClient;
extern PubSubClient client_mqtt;
extern Manager _manager;

// Set web server port number to 81
extern WiFiServer server;

// Connection states for better state management
enum ConnectionState {
    DISCONNECTED,
    CONNECTING_WIFI,
    WIFI_CONNECTED,
    CONNECTING_MQTT,
    FULLY_CONNECTED
};

class Wifi_Mqtt
{
private:
    const char *ssid_default = "HelpSmart";
    const char *password_default = "HelpSmart";
    const char *mqtt_user = "device1.helpmedica";
    const char *mqtt_pass = "device1.helpmedica";
    
    // Connection management parameters
    ConnectionState connectionState = DISCONNECTED;
    unsigned long lastConnectionAttempt = 0;
    unsigned long reconnectInterval = 1000; // Start with 1 second
    const unsigned long maxReconnectInterval = 60000; // Max 1 minute between attempts
    int wifiReconnectAttempts = 0;
    int mqttReconnectAttempts = 0;
    const int maxReconnectAttempts = 10; // Reset reconnect strategy after this many attempts
    bool wifiConnectedBefore = false;

public:
    bool verify_internet();
    void conectar_wifi(char *p_nombre_wifi, char *p_clave_wifi);
    void reconnect(char *p_nombre_wifi, char *p_clave_wifi);
    
    // New methods for better connection management
    bool manage_connections(char *p_nombre_wifi, char *p_clave_wifi);
    ConnectionState getConnectionState() { return connectionState; }
    void resetConnectionParams();
};

#endif