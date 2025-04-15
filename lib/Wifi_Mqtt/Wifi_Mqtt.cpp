#include "Wifi_Mqtt.h"

static bool deviceJustStarted = true;

bool Wifi_Mqtt::verify_internet()
{
    Serial.println("Verificando conexión a internet");

    HTTPClient http;
    http.begin("http://www.google.com");
    int httpResponseCode = http.GET();
    http.end();

    if (httpResponseCode == 200)
    {
        Serial.println("Conectado a internet");
        return true;
    }
    else
    {
        Serial.printf("No conectado a internet, código de respuesta: %d\n", httpResponseCode);
        return false;
    }
}

void Wifi_Mqtt::resetConnectionParams() {
    reconnectInterval = 1000;
    wifiReconnectAttempts = 0;
    mqttReconnectAttempts = 0;
    Serial.println("Parámetros de reconexión restablecidos");
}

void Wifi_Mqtt::conectar_wifi(char *p_nombre_wifi, char *p_clave_wifi)
{
    // First connection attempt is special - we give it more time and attention
    if (deviceJustStarted) {
        Serial.print("Primer intento de conexión WiFi a: ");
        Serial.println(p_nombre_wifi);
        
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.setHostname("HelpSmart");
            WiFi.begin(p_nombre_wifi, p_clave_wifi);
            
            // Initial connection - we wait a bit longer
            int intentos = 0;
            connectionState = CONNECTING_WIFI;
            
            while (WiFi.status() != WL_CONNECTED && !server_status && intentos < 30) {
                use_led = false;
                Serial.print('.');
                delay(1000);
                intentos++;
            }
            
            // If primary WiFi connection fails, try the default one
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("\nFallo en conexión primaria, intentando red predeterminada...");
                WiFi.disconnect();
                delay(100);
                WiFi.begin(ssid_default, password_default);
                
                intentos = 0;
                while (WiFi.status() != WL_CONNECTED && !server_status && intentos < 20) {
                    use_led = false;
                    Serial.print('-');
                    delay(1000);
                    intentos++;
                }
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nConexión WiFi establecida!");
                Serial.print("Dirección IP: ");
                Serial.println(WiFi.localIP());
                connectionState = WIFI_CONNECTED;
                wifiConnectedBefore = true;
            } else {
                Serial.println("\nFallo en ambas conexiones WiFi.");
                connectionState = DISCONNECTED;
            }
            
            deviceJustStarted = false;
        }
    } 
    // Normal reconnection logic - non-blocking
    else {
        // Only try to reconnect if we're not currently in the middle of connecting
        if (connectionState == DISCONNECTED && millis() - lastConnectionAttempt > reconnectInterval) {
            Serial.print("Intento de reconexión WiFi a: ");
            Serial.println(p_nombre_wifi);
            
            if (server_status == false) {
                use_led = false;
            }
            
            WiFi.disconnect();
            delay(100);
            WiFi.setHostname("Helpsmart");
            
            // Try primary WiFi first, unless we've had too many failed attempts
            bool tryPrimary = wifiReconnectAttempts % 3 != 2; // Try default every 3rd attempt
            
            if (tryPrimary) {
                WiFi.begin(p_nombre_wifi, p_clave_wifi);
                Serial.println("Intentando red primaria");
            } else {
                WiFi.begin(ssid_default, password_default);
                Serial.println("Intentando red predeterminada");
            }
            
            lastConnectionAttempt = millis();
            connectionState = CONNECTING_WIFI;
            wifiReconnectAttempts++;
            
            // Implement exponential backoff for reconnection attempts
            if (wifiReconnectAttempts > 1) {
                reconnectInterval = min(reconnectInterval * 2, maxReconnectInterval);
                Serial.printf("Próximo intento en %d ms\n", reconnectInterval);
            }
            
            // After several attempts, consider a reset or alternative strategy
            if (wifiReconnectAttempts >= maxReconnectAttempts) {
                Serial.println("Múltiples intentos fallidos, reiniciando parámetros de conexión");
                resetConnectionParams();
            }
        }
        
        // Check if we've connected
        if (connectionState == CONNECTING_WIFI && WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi conectado!");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            connectionState = WIFI_CONNECTED;
            wifiConnectedBefore = true;
            resetConnectionParams(); // Reset backoff on successful connection
        }
    }
}

//*****************************
//***    CONEXION MQTT      ***
//*****************************
void Wifi_Mqtt::reconnect(char *p_nombre_wifi, char *p_clave_wifi)
{
    // Only attempt MQTT connection if WiFi is connected
    if (WiFi.status() == WL_CONNECTED && !client_mqtt.connected()) {
        
        // Only try to connect at appropriate intervals
        if (connectionState != CONNECTING_MQTT && millis() - lastConnectionAttempt > reconnectInterval) {
            if (server_status == false) {
                use_led = false;
            }
            
            Serial.print("Intentando conexión MQTT al broker: ");
            Serial.print(mqtt_server);
            Serial.print(":");
            Serial.println(mqtt_port);
            
            // Set the connection state before attempting
            connectionState = CONNECTING_MQTT;
            lastConnectionAttempt = millis();
            
            // Try to connect
            if (client_mqtt.connect(_manager.id().c_str(), mqtt_user, mqtt_pass)) {
                Serial.println("¡Conectado a MQTT!");
                
                // Subscribe to topics
                if (client_mqtt.subscribe(topic_sub.c_str())) {
                    Serial.print("Suscrito a: ");
                    Serial.println(topic_sub);
                } else {
                    Serial.println("Error al suscribirse al tópico");
                }
                
                connectionState = FULLY_CONNECTED;
                resetConnectionParams(); // Reset backoff on successful connection
            } else {
                mqttReconnectAttempts++;
                Serial.print("Fallo en conexión MQTT, rc=");
                Serial.println(client_mqtt.state());
                
                // Verify internet connection - if we have WiFi but no internet, we should reconnect WiFi
                if (!verify_internet()) {
                    Serial.println("WiFi conectado, pero sin internet - desconectando WiFi");
                    WiFi.disconnect();
                    connectionState = DISCONNECTED;
                    return;
                }
                
                // Implement exponential backoff for reconnection attempts
                if (mqttReconnectAttempts > 1) {
                    reconnectInterval = min(reconnectInterval * 2, maxReconnectInterval);
                    Serial.printf("Próximo intento MQTT en %d ms\n", reconnectInterval);
                }
                
                // After several attempts, consider a reset
                if (mqttReconnectAttempts >= maxReconnectAttempts) {
                    Serial.println("Múltiples intentos MQTT fallidos, reiniciando parámetros de conexión");
                    resetConnectionParams();
                }
            }
        }
    } else if (WiFi.status() != WL_CONNECTED) {
        // If WiFi is disconnected but we were previously connected to MQTT,
        // update the state accordingly
        if (connectionState == FULLY_CONNECTED || connectionState == CONNECTING_MQTT) {
            connectionState = DISCONNECTED;
            Serial.println("Conexión WiFi perdida - necesita reconexión");
        }
    }
}

// New consolidated connection management function
bool Wifi_Mqtt::manage_connections(char *p_nombre_wifi, char *p_clave_wifi) {
    // Check for WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        if (connectionState != DISCONNECTED && connectionState != CONNECTING_WIFI) {
            Serial.println("WiFi desconectado. Cambiando estado.");
            connectionState = DISCONNECTED;
        }
        conectar_wifi(p_nombre_wifi, p_clave_wifi);
        return false;
    } 
    // WiFi connected, check MQTT
    else if (!client_mqtt.connected()) {
        if (connectionState != CONNECTING_MQTT && connectionState != WIFI_CONNECTED) {
            connectionState = WIFI_CONNECTED;
        }
        reconnect(p_nombre_wifi, p_clave_wifi);
        return false;
    } 
    
    // Both WiFi and MQTT are connected
    if (connectionState != FULLY_CONNECTED) {
        Serial.println("Conexión completa establecida (WiFi + MQTT)");
        connectionState = FULLY_CONNECTED;
        resetConnectionParams();
    }
    
    return true; // Both WiFi and MQTT connected
}