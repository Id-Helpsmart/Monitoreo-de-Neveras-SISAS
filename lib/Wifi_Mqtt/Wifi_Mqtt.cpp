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
        WiFi.setHostname("Helpsmart");

        // esp_wifi_set_max_tx_power(80); // Establecer la potencia máxima a 20 dBm
        // WiFi.config(ip, gateway, subnet);  // Establecer conexión con una IP fija
        WiFi.begin(p_nombre_wifi, p_clave_wifi);
        Serial.println("Esperando conexión WiFi");
        
        if (deviceJustStarted)
        {
            while (WiFi.status() != WL_CONNECTED)
            {
                use_led = false;
                Serial.print('.');
                delay(1000);
                if (intentos <= 30)
                    intentos++;
                else
                    ESP.restart();
            }
            Serial.println(WiFi.localIP());
            deviceJustStarted = false;
        }
        else
        {

            if (WiFi.status() != WL_CONNECTED)
            {
                if (server_status == false)
                    use_led = false;
                Serial.print('.');
                delay(1000);
            }
            else
                Serial.println(WiFi.localIP());
        }
    }
}

//*****************************
//***    CONEXION MQTT      ***
//*****************************
void Wifi_Mqtt::reconnect(char *p_nombre_wifi, char *p_clave_wifi)
{
    if (!client_mqtt.connected())
    {
        if (server_status == false)
            use_led = false;

        Serial.print("Intentando conexion Mqtt...");
        //  Intentamos conectar
        if (client_mqtt.connect(_manager.id().c_str(), mqtt_user, mqtt_pass))
        {
            Serial.println("Conectado!");

            // Nos suscribimos
            // if (client_mqtt.subscribe(topic_pub.c_str()))
            // Serial.println(topic_pub);
            if (client_mqtt.subscribe(topic_sub.c_str()))
                Serial.println(topic_sub);
            // if (client_mqtt.subscribe(topic_rtc.c_str()))
            // Serial.println(topic_rtc);
            else
            {
                Serial.println("x");
            }
        }
        else
        {
            // if (WiFi.status() == WL_CONNECTED && conteo_mqtt < 5) // hay 5 intentos de reconexión, 1 cada 2 segundos
            if (WiFi.status() == WL_CONNECTED) // hay 5 intentos de reconexión, 1 cada 2 segundos
            {
                // conteo_mqtt++;
                if (verify_internet() == false)
                {
                    Serial.println("Conectado, pero sin internet");
                    WiFi.disconnect();
                }
                Serial.println("fallo en la comunicacion con el servidor broker");
                Serial.println(client_mqtt.state());
                // Serial.println("Intentamos de nuevo en 2 segundos");
                delay(1000);
            }
            else
            {
                Serial.println("fallo del wifi");
                Serial.println("Reconectar WiFi");
                conectar_wifi(p_nombre_wifi, p_clave_wifi);
            }
        }
    }
}