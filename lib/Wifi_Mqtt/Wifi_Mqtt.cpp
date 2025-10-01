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

void Wifi_Mqtt::conectar_wifi(char *p_nombre_wifi, char *p_clave_wifi)
{
    WiFi.mode(WIFI_STA);

    int intentos = 0;
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Conectando a red wifi \n");
        Serial.println(p_nombre_wifi);
        Serial.println(p_clave_wifi);
        // WiFi.setHostname("Helpsmart");

        // esp_wifi_set_max_tx_power(80); // Establecer la potencia máxima a 20 dBm
        // WiFi.config(ip, gateway, subnet);  // Establecer conexión con una IP fija
        WiFi.begin(p_nombre_wifi, p_clave_wifi);
        Serial.println("Esperando conexión WiFi");

        // while (WiFi.status() != WL_CONNECTED && !server_status)
        // while (WiFi.status() != WL_CONNECTED)
        while (WiFi.status() != WL_CONNECTED && !completed_time && !emergency)
        {
            if (!server_status)
                use_led = false;
                
            Serial.print('.');
            delay(1000);
            if (intentos <= 10)
            {
                intentos++;
            }
            else
            {
                Serial.println("Máximos intentos alcanzados, restableciendo WiFi...");
                WiFi.disconnect(true);
                WiFi.begin(p_nombre_wifi, p_clave_wifi);
                intentos = 0;
            }
        }
        Serial.println(WiFi.localIP());
    }
}

//*****************************
//***    CONEXION MQTT      ***
//*****************************
void Wifi_Mqtt::reconnect(char *p_nombre_wifi, char *p_clave_wifi)
{
    static int reconnection_attempts = 0;
    static unsigned long last_disconnect_time = 0;
    unsigned long current_time = millis();

    // Si han pasado más de 30 segundos desde el último intento fallido, reiniciar el contador
    if (current_time - last_disconnect_time > 30000)
    {
        reconnection_attempts = 0;
    }

    if (!client_mqtt.connected())
    {
        // Limitar la frecuencia de intentos de reconexión basado en intentos fallidos
        if (reconnection_attempts > 0)
        {
            int delay_time = min(reconnection_attempts * 1000, 5000); // Máximo 5 segundos de retraso
            if (current_time - last_disconnect_time < delay_time)
            {
                return; // No intentar reconectar demasiado rápido
            }
        }

        Serial.print("Intentando conexion Mqtt...");

        // Desconectar antes de intentar una nueva conexión para liberar recursos
        client_mqtt.disconnect();
        delay(100); // Un pequeño retraso para asegurar que la desconexión se completa

        //  Intentamos conectar con un ID único usando timestamp
        String clientId = _manager.id() + String(millis() % 1000);
        if (client_mqtt.connect(clientId.c_str(), mqtt_user, mqtt_pass))
        {
            Serial.println("Conectado!");
            reconnection_attempts = 0; // Resetear contador de intentos

            // Nos suscribimos
            if (client_mqtt.subscribe(topic_sub.c_str()))
                Serial.println(topic_sub);
            else
                Serial.println("Error al suscribirse");
        }
        else
        {
            reconnection_attempts++;
            last_disconnect_time = current_time;

            if (WiFi.status() == WL_CONNECTED)
            {
                if (verify_internet() == false)
                {
                    Serial.println("Conectado, pero sin internet");
                    delay(1000);
                    WiFi.disconnect();
                    delay(500);
                    WiFi.begin(p_nombre_wifi, p_clave_wifi);
                }
                Serial.print("fallo en la comunicacion con el servidor broker: ");
                Serial.println(client_mqtt.state());

                // Si hay demasiados errores de "No more processes", reiniciar WiFi
                if (reconnection_attempts > 5)
                {
                    Serial.println("Demasiados intentos fallidos, reiniciando WiFi...");
                    WiFi.disconnect(true);
                    delay(1000);
                    WiFi.begin(p_nombre_wifi, p_clave_wifi);
                    delay(2000);
                }
            }
            else
            {
                Serial.println("fallo del wifi");
                Serial.println("Reconectar WiFi");
                reconnection_attempts = 0; // Resetear para WiFi
                conectar_wifi(p_nombre_wifi, p_clave_wifi);
            }
        }
    }
}