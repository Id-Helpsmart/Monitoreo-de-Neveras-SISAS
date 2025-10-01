#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Firebase_ESP_Client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include "esp_timer.h" // Para el timer de alto nivel de ESP32

#include "BLS_AP.h"
#include "Sensores.h"
#include "Wifi_Mqtt.h"
#include "timestamp.h"
#include "funciones_spiffs.h"

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 2. Define the API Key */
#define API_KEY "AIzaSyATBJUao_YkC7o84tNZ25klL8IIEZ-7dNU"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "sensortemperatura@gmail.com"
#define USER_PASSWORD "9001295137"

#define STORAGE_BUCKET_ID "alerta--y-emergencia-policia.appspot.com"

#define LED 25           // Pin de salida para el led de la tarjeta
#define BUZZER 19        // Pin de salida para el buzzer
#define BOTON_COD_AZUL 2 // Pulsador 2: pin GPIO2. PARA PUNTO DE ACCESO

#define WDT_TIMEOUT 600000

#define STACK_ALARM 2000
#define STACK_UPDATE 8000
#define STACK_SENSORS 10000
#define STACK_FLASHING_LED 5000

// Definir mutex para proteger acceso a registros
SemaphoreHandle_t mutexSensorsStruct;

typedef struct // estructura para almacenar la frecuencia con que se publican datos en el servidor o con que se envían por comunicación LoRa
{
    char config_nombre_wifi[TAMANO_CONFIG_GEN];
    char config_clave_wifi[TAMANO_CONFIG_GEN];
    char configcompany_name[TAMANO_CONFIG_GEN];
    char configarea_name[TAMANO_CONFIG_GEN];
} estructura1;

typedef struct // estructura para almacenar la frecuencia con que se publican datos en el servidor o con que se envían por comunicación LoRa
{
    char frecuencia_pub[10];
} estructura2;

typedef struct // estructura para almacenar los niveles de alarma de los sensores
{
    char nivel_min[10];
    char nivel_max[10];
} estructura3;

estructura1 datos_recibidos_gen; // variable para almacenar datos configuración general
estructura2 datos_recibidos_sen; // variable para almacenar datos configuración de sensores
estructura3 datos_recibidos_ala; // variable para almacenar datos niveles de alarma

const char *fileData = "/device-data.txt";
// const char *fileData = "/data.csv";
// const char *password = "helpmedica$";

//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char *mqtt_server = "iotm.helpmedica.com";
const int mqtt_port = 1884; // broker de emergencias

char name_card[50];
char previous_temperature[9] = "*";

bool use_led = false;
bool trouble = false;
bool advance = false;
bool firmware = false;
bool updating = false;
bool out_of_range = false;
bool reconnection = false;
bool server_close = false;
bool create_alarm = false;
bool taskCompleted = false;
bool server_status = false;
bool posted_message = false;
bool trouble_publish = false;
bool message_received = false;
bool download_firmware = false;
bool flag_access_point = false;
bool flag_callback_broker = false;
bool firstMessageToPublish = true;
bool synchronizedTimestamp = false;
bool configuration_updated = false;

volatile bool emergency = false;
volatile bool completed_time = false;

long lastMsg = 0;
long lastMsgRead = 0;
long last_alto_bajo = 0;
long last_measurement = 0;
long last_reconnection = 0;
static unsigned long lastSyncTime = 0;

// Variables para el timer ESP32
esp_timer_handle_t publish_timer = NULL;
bool input_energy = false;

float temperature = 0;
float battery_level = 0;
float publish_freq_num = 5000; // frecuencia de publicación de datos de sensores (por defecto cada 5 seg para verificar lectura correcta desde SPIFFS)

String location;
String topic_tro;
String topic_upd;
String topic_sub;
String topic_pub;
String area_name;
String company_name;
String message_trouble;

JsonDocument doc_config;

DS18B20Data readData;

// bluetooth
BLS_AP _BLS_AP;
WiFiClient TcpClient;
WiFiClient espClient;
WiFiServer server(81);
PubSubClient client_mqtt(espClient);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Manager _manager;
Sensores _Sensors;
Wifi_Mqtt _Wifi_Mqtt;
Timestamp _timestamp;
funciones_spiffs _funciones_spiffs;

TaskHandle_t Task1_alarm;
TaskHandle_t TaskSensors;
TaskHandle_t Task3_update;
TaskHandle_t Task2_flashing_led;

eTaskState state_update;
eTaskState state_sensors;
eTaskState state_alarm_task;
eTaskState state_flashing_led;

StaticTask_t dates_alarm;
StaticTask_t dates_update;
StaticTask_t dates_sensors;
StaticTask_t dates_flashing_led;

StackType_t stack_alarm[STACK_ALARM];
StackType_t stack_update[STACK_UPDATE];
StackType_t stack_sensors[STACK_SENSORS];
StackType_t stack_flashing_led[STACK_FLASHING_LED];

TickType_t last_sensors_lock_time = pdMS_TO_TICKS(10);
TickType_t led_flashing_lock_time = pdMS_TO_TICKS(500);
TickType_t firmware_update_lock_time = pdMS_TO_TICKS(10);

// hw_timer_t *timer_zero_AP = NULL;

portMUX_TYPE free_zone_timers = portMUX_INITIALIZER_UNLOCKED;  // variable para establecer zona de código libre de interrupciones durante interrupciones de temporizador
portMUX_TYPE free_zone_buttons = portMUX_INITIALIZER_UNLOCKED; // variable para establecer zona de código libre de interrupciones durante interrupciones de pulsación de botones

void IRAM_ATTR onPublishTimer(void *arg);
void setupPublishTimer(uint64_t interval_us);

void files_read();
void create_files();
void Access_Point();
void Cod_Access_Point();
String createJsonTrouble();
String createJsonUpdate(String msg);
// bool isStructNull(const DS18B20Data &);
void Sensors(void *);
void Alarm(void *pvParameters);
void TaskUpdate(void *pvParameters);
void Flashing_Led(void *pvParameters);
// DynamicJsonDocument json(bool current_or_saved); // Función para armar el Json
String createJsonSensor(bool is_real_time);
void callback(char *topic, byte *payload, unsigned int length);

//*****************************
//***        SETUP          ***
//*****************************
void setup()
{
    pinMode(LED, OUTPUT);    //
    pinMode(BUZZER, OUTPUT); // Declarando buzzer como salida pin de led en tarjeta
    pinMode(BOTON_COD_AZUL, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(BOTON_COD_AZUL), Cod_Access_Point, RISING); // LOW/HIGH/FALLING/RISING/CHANGE
    Serial.begin(115200);
    Serial.println("Version Bluetooth + OTA 30-09-2025"); // Iniciar la comunicaión serial en 115200 baudios                                                        // Iniciar la comunicaión serial en 115200 baudios

    digitalWrite(BUZZER, LOW);
    lastSyncTime = millis();

    create_files();
    files_read();

    // Crear mutex
    mutexSensorsStruct = xSemaphoreCreateMutex();
    if (mutexSensorsStruct == NULL)
    {
        Serial.println("Error al crear mutex");
        while (1)
            ;
    }

    Task2_flashing_led = xTaskCreateStaticPinnedToCore(Flashing_Led, "flashing_led", STACK_FLASHING_LED, NULL, 2, stack_flashing_led, &dates_flashing_led, 0);
    TaskSensors = xTaskCreateStaticPinnedToCore(Sensors, "Sensors", STACK_SENSORS, NULL, 2, stack_sensors, &dates_sensors, 0);
    delay(500);

    // Inicializar el timer para la publicación periódica
    setupPublishTimer(publish_freq_num * 1000);

    _Wifi_Mqtt.conectar_wifi(datos_recibidos_gen.config_nombre_wifi, datos_recibidos_gen.config_clave_wifi);

    client_mqtt.setServer(mqtt_server, mqtt_port);
    client_mqtt.setCallback(callback); // si la descomento, aparece el problema "invalid use of non-static member function"
}

//*****************************
//***         LOOP          ***
//*****************************
void loop()
{
    state_flashing_led = eTaskGetState(Task2_flashing_led);

    if (state_flashing_led != eRunning || state_flashing_led != eReady || state_flashing_led != eBlocked) // si el estado es distinto a corriendo, listo o bloqueado
        vTaskResume(Task2_flashing_led);

    if (eTaskGetState(TaskSensors) == eSuspended)
        vTaskResume(TaskSensors);

    if (configuration_updated)
        _funciones_spiffs.process_file(doc_config);

    if (server_close == true)
    {
        _BLS_AP.deInitBLE();
        server_close = false;
        server_status = false;
        use_led = false;                             // Detener el parpadeo del LED cuando se cierra el servidor Bluetooth
        led_flashing_lock_time = pdMS_TO_TICKS(500); // Restaurar la frecuencia normal de parpadeo
    }

    if (firmware == true)
    {
        firmware = false;
        updating = true;
        Task3_update = xTaskCreateStaticPinnedToCore(TaskUpdate, "update", STACK_UPDATE, NULL, 3, stack_update, &dates_update, 1);
        // delay(500);
        led_flashing_lock_time = pdMS_TO_TICKS(50); // parpadeo cuando se conectó un cliente al punto de acceso
    }

    if (flag_callback_broker == true)
    {
        files_read();
        emergency = true;
        flag_callback_broker = false;
    }

    if (millis() - last_reconnection >= 1000)
    {
        _Wifi_Mqtt.conectar_wifi(datos_recibidos_gen.config_nombre_wifi, datos_recibidos_gen.config_clave_wifi);
        last_reconnection = millis();
    }

    // Timer de sincronización
    if (millis() - lastSyncTime >= 24L * 60L * 60L * 1000L) // 24 hours in milliseconds
    {
        _timestamp.synchronizeTimestamp();
        lastSyncTime = millis();
    }

    if (out_of_range && !create_alarm)
    {
        Task1_alarm = xTaskCreateStaticPinnedToCore(Alarm, "Alarm", STACK_ALARM, NULL, 2, stack_alarm, &dates_alarm, 0);
        delay(500);
        create_alarm = true;
    }
    else if (!out_of_range && create_alarm)
    {
        digitalWrite(BUZZER, LOW);
        vTaskDelete(Task1_alarm);
        create_alarm = false;
    }

    // Comprobar si el timer se ha disparado o si hay una emergencia
    // if (timer_triggered || emergency)
    if (completed_time || emergency)
    {
        // if (timer_triggered)
        //     Serial.println("Timer triggered");
        // else
        //     Serial.println("Emergency triggered");

        if (!trouble)
            completed_time = true, trouble = false, Serial.println("Completed time");
        else
            trouble_publish = true, Serial.println("Trouble publish");

        if (emergency)
            emergency = false;

        // timer_triggered = false; // Resetear el flag del timer
    }

    if (!updating)
    {
        if (WiFi.status() == WL_CONNECTED)
        // if (wifi_connected && mqtt_connected)
        {
            if (!client_mqtt.connected())
                _Wifi_Mqtt.reconnect(datos_recibidos_gen.config_nombre_wifi, datos_recibidos_gen.config_clave_wifi);
            else
            {
                if (!synchronizedTimestamp)
                {
                    _timestamp.synchronizeTimestamp();
                    if (_timestamp.getTime().startsWith("1970"))
                        synchronizedTimestamp = false;
                    else
                        synchronizedTimestamp = true;
                }

                if (server_status == false && !flag_access_point)
                    use_led = false; // Solo encender LED cuando hay un punto de acceso activo

                if (trouble_publish)
                {
                    // createJsonTrouble();
                    Serial.println("Publish trouble");
                    if (client_mqtt.publish(topic_tro.c_str(), createJsonTrouble().c_str()) == true)
                        trouble_publish = false;
                }

                if (completed_time || (firstMessageToPublish && !trouble))
                {
                    if (firstMessageToPublish)
                        firstMessageToPublish = false, Serial.println("First message to publish");

                    if (client_mqtt.publish(topic_pub.c_str(), createJsonSensor(true).c_str()) == true)
                        Serial.println("Publish");
                    else
                        _funciones_spiffs.save_sensor_data(_timestamp.getTime(), readData, input_energy);
                    // _funciones_spiffs.save_data(_timestamp.getTime(), readData, input_energy);
                    //_funciones_spiffs.Almacenar_registro_sensores(_timestamp.getTime(), &readData, input_energy);

                    completed_time = false;
                }
                if (SPIFFS.exists(fileData) && (millis() - lastMsgRead) > 1000)
                {
                    String _createJsonSensor = createJsonSensor(false);
                    if (_createJsonSensor != "")
                    {
                        if (client_mqtt.publish(topic_pub.c_str(), _createJsonSensor.c_str()) == true)
                        {
                            DS18B20Saved _savedData;
                            _funciones_spiffs.read_sensor_register(false, _savedData);
                            Serial.println("Publish saved data");
                        }
                        // _funciones_spiffs.delete_first_data();
                    }
                    // else
                    //     _funciones_spiffs.delete_first_data();
                    lastMsgRead = millis();
                }
            }
        }
        else
        {
            if (completed_time)
            {
                _funciones_spiffs.save_sensor_data(_timestamp.getTime(), readData, input_energy);
                completed_time = false;
            }
        }
        client_mqtt.loop();
    }
}

// DynamicJsonDocument json(bool current_or_saved)
String createJsonSensor(bool is_real_time)
{
    // char timestamp_saved[22] = "*";
    // float temperature_saved = 0;
    // int energy_saved = 0;
    // int door_saved = 0;
    DS18B20Saved savedData = {};

    if (is_real_time == false)
        _funciones_spiffs.read_sensor_register(true, savedData);

    if (!is_real_time && strcmp(savedData._DS18B20Data.time, "*") == 0)
        return "";

    Serial.println("Creating JSON");
    Serial.print(savedData._DS18B20Data.time);
    Serial.print(" - ");
    Serial.println(savedData._DS18B20Data.temperature);

    JsonDocument doc;
    doc["empresa"] = company_name;
    doc["area"] = area_name;
    doc["pcbname"] = name_card;
    doc["mac"] = _manager.id();
    doc["is_real_time"] = is_real_time;
    is_real_time ? doc["timestamp"] = _timestamp.getTime() : doc["timestamp"] = savedData._DS18B20Data.time;
    JsonArray sensors = doc.createNestedArray("sensor");
    //----------------------------------------------------------------------------------------------
    // Sensor 0

    JsonObject sensor1 = sensors.createNestedObject();
    sensor1["type"] = 13;
    sensor1["name"] = "T";
    sensor1["number"] = 1; // sensor number
    JsonArray s1values = sensor1.createNestedArray("values");
    JsonObject s1value0 = s1values.createNestedObject();
    s1value0["type"] = keyValueToString(keyValues::TEMPERATURE);
    is_real_time ? s1value0["value"] = atof(readData.temperature) : s1value0["value"] = atof(savedData._DS18B20Data.temperature);
    s1value0["unit"] = "°C";
    s1value0["Lmin"] = alarmLimits[keyValues::TEMPERATURE].min;
    s1value0["Lmax"] = alarmLimits[keyValues::TEMPERATURE].max;

    JsonObject sensor101 = sensors.createNestedObject();
    sensor101["type"] = 10;
    sensor101["name"] = "IND";
    sensor101["number"] = 1; // número de switch. se le suma 1 para que en el broker se identifique desde switch 1 en adelante
    JsonArray s101values = sensor101.createNestedArray("values");
    JsonObject s101value0 = s101values.createNestedObject();
    s101value0["type"] = "ENERGIA";
    is_real_time ? s101value0["value"] = input_energy ? 1 : 0 : s101value0["value"] = atoi(savedData.energy);
    // s101value0["value"] = input_energy;)
    s101value0["unit"] = "BOOL";
    s101value0["Lmin"] = 0;
    s101value0["Lmax"] = 1;

    JsonObject sensor100 = sensors.createNestedObject();
    sensor100["type"] = 100;
    sensor100["name"] = "BAT";
    sensor100["number"] = 1; // número de sensor
    JsonArray s100values = sensor100.createNestedArray("values");
    JsonObject s100value0 = s100values.createNestedObject();
    s100value0["type"] = "BATERIA";
    s100value0["value"] = battery_level;
    s100value0["unit"] = "%";
    s100value0["Lmin"] = 10;
    s100value0["Lmax"] = 100;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

String createJsonTrouble()
{
    JsonDocument doc;
    JsonArray troubleArray = doc.createNestedArray("trouble");

    JsonObject trouble = troubleArray.createNestedObject();
    trouble["empresa"] = company_name;
    trouble["area"] = area_name;
    trouble["mac"] = _manager.id();
    trouble["pcbname"] = name_card;
    trouble["timestamp"] = _timestamp.getTime();
    trouble["msg"] = message_trouble;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

String createJsonUpdate(String msg)
{
    JsonDocument doc; // Tamaño más pequeño para update
    JsonArray updateArray = doc.createNestedArray("update");

    JsonObject update = updateArray.createNestedObject();
    update["empresa"] = company_name;
    update["area"] = area_name;
    update["mac"] = _manager.id();
    update["pcbname"] = name_card;
    update["timestamp"] = _timestamp.getTime();
    update["msg"] = msg;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

//*****************************
//***       CALLBACK        ***
//*****************************
void callback(char *topic, byte *payload, unsigned int length)
{
    if (strcmp(topic, topic_pub.c_str()) != 0)
    {
        String incoming = "";

        for (int i = 0; i < length; i++)
            incoming += (char)payload[i];

        incoming.trim();

        JsonDocument doc;

        // Parsea el JSON
        DeserializationError error = deserializeJson(doc, incoming);

        // Verifica si ha ocurrido algún error durante el parsing
        if (error)
            return;
        else
            _funciones_spiffs.process_file(doc);
    }
    // else if (posted_message)
    //     message_received = true;
}

// Funciones de interrupción por hardware
// ##################################################################

void IRAM_ATTR Cod_Access_Point() // función a ejecutar cuando se presiona el boton de código azul. esta se ejecuta desde la memoria ram interna
{
    portENTER_CRITICAL(&free_zone_buttons);
    flag_access_point = true;
    portEXIT_CRITICAL(&free_zone_buttons);
}
// ###################################################################

void Access_Point()
{
    unsigned long buttonPressTime = millis();
    while (digitalRead(BOTON_COD_AZUL) == HIGH)
    {
        // Serial.println("Access Point");
        if (millis() - buttonPressTime > 2000)
        {
            use_led = true;
            led_flashing_lock_time = pdMS_TO_TICKS(100); // Indicar que está abierto el punto de acceso
            _BLS_AP.Access_Point(&server_status);
            // Cuando el servidor Bluetooth se cierra, detener el parpadeo del LED
            if (!server_status)
            {
                use_led = false;
                led_flashing_lock_time = pdMS_TO_TICKS(500); // Restaurar frecuencia normal
            }
            break;
        }
    }
    flag_access_point = false;
}

void Sensors(void *pvParameters)
{
    // Serial.print("Task2 running on core ");
    // Serial.println(xPortGetCoreID());
    TickType_t last_execution_time;

    _Sensors.Setup_Sen();

    if (xSemaphoreTake(mutexSensorsStruct, portMAX_DELAY))
        _Sensors.readData(readData), xSemaphoreGive(mutexSensorsStruct); // Liberar el semáforo

    for (;;)
    {
        if (flag_access_point == true && server_status == false)
        {
            Serial.println("Access Point");
            Access_Point(); // Ya que esta tarea se ejecuta constantemente, se verifica si se activó el modo configuración
        }

        if (millis() - last_measurement >= 1000 && updating == false)
        {
            _Sensors.readData(readData);

            last_measurement = millis();

            std::vector<std::pair<float, keyValues::Type>> valuesToCheck = {
                {atof(readData.temperature), keyValues::TEMPERATURE},
            };

            for (const auto &value : valuesToCheck)
            {
                // Serial.println(value.first);
                if (value.first < alarmLimits[value.second].min || value.first > alarmLimits[value.second].max)
                {
                    Serial.println("Out of range");
                    if (!out_of_range)
                        emergency = true, out_of_range = true, Serial.println("Emergency");
                }
                else
                    out_of_range = false;
            }
        }

        input_energy = _Sensors.Input_Energy(&emergency);

        if (input_energy == '0')
            battery_level = _Sensors.Battery_Level(&emergency);
        else
            battery_level = 100;

        last_execution_time = xTaskGetTickCount();
        vTaskDelayUntil(&last_execution_time, last_sensors_lock_time);
    }
}

void Alarm(void *pvParameters)
{

    for (;;)
    {
        if (out_of_range == true)
        {
            long alto_bajo = millis();
            if (alto_bajo - last_alto_bajo < 500)
                digitalWrite(BUZZER, HIGH);
            if (alto_bajo - last_alto_bajo > 500)
                digitalWrite(BUZZER, LOW);
            if (alto_bajo - last_alto_bajo > 1000)
                last_alto_bajo = alto_bajo;
        }
        else
        {
            emergency = true;
            last_alto_bajo = millis();
            digitalWrite(BUZZER, LOW);
        }
        vTaskDelay(10);
    }
}

// The Firebase Storage download callback function
void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
    if (info.status == fb_esp_fcs_download_status_init)
    {
        download_firmware = true;
        Serial.printf("Downloading firmware %s (%d)\n", info.remoteFileName.c_str(), info.fileSize);
    }
    else if (info.status == fb_esp_fcs_download_status_download)
        Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    else if (info.status == fb_esp_fcs_download_status_complete)
    {
        client_mqtt.publish(topic_upd.c_str(), createJsonUpdate("Update firmware completed").c_str());
        Serial.println("Update firmware completed.");
        Serial.println();
        Serial.println("Restarting...\n\n");
        ESP.restart();
    }
    else if (info.status == fb_esp_fcs_download_status_error)
    {
        client_mqtt.publish(topic_upd.c_str(), createJsonUpdate("Download firmware failed").c_str());
        Serial.printf("Download firmware failed, %s\n", info.errorMsg.c_str());
        Serial.println("Restarting...\n\n");
        ESP.restart();
    }
}

void TaskUpdate(void *pvParameters)
{
    TickType_t ultimo_tiempo_ejecucion;

    esp_task_wdt_init(WDT_TIMEOUT, false); // habilita el pánico para que ESP32 se reinicie
    esp_task_wdt_add(NULL);                // agregar hilo actual al reloj WDT

    // Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

    Firebase.begin(&config, &auth);

    while (1)
    {
        if (Firebase.ready() && !taskCompleted)
        {
            taskCompleted = true;

            // If you want to get download url to use with your own OTA update process using core update library,
            // see Metadata.ino example

            Serial.println("\nDownload firmware file...\n");

            if (!Firebase.Storage.downloadOTA(&fbdo, STORAGE_BUCKET_ID, location.c_str(), fcsDownloadCallback))
                Serial.println(fbdo.errorReason());
        }

        ultimo_tiempo_ejecucion = xTaskGetTickCount();
        vTaskDelayUntil(&ultimo_tiempo_ejecucion, firmware_update_lock_time);
    }
}

void Flashing_Led(void *pvParameters)
{
    TickType_t ultimo_tiempo_ejecucion1;

    esp_task_wdt_init(WDT_TIMEOUT, false); // habilita el pánico para que ESP32 se reinicie
    esp_task_wdt_add(NULL);                // agregar hilo actual al reloj WDT

    bool led_on_off = false;

    while (1)
    {
        if (use_led == true)
        {
            led_on_off = !led_on_off;
            if (led_on_off == true)
                digitalWrite(LED, HIGH);
            else
                digitalWrite(LED, LOW);
        }
        else
            digitalWrite(LED, LOW);
        ultimo_tiempo_ejecucion1 = xTaskGetTickCount();
        vTaskDelayUntil(&ultimo_tiempo_ejecucion1, led_flashing_lock_time);
    }
}

void create_files()
{
    if (!SPIFFS.begin(true)) // debe iniciarse antes que la función "conectar_wifi", porque inicia el uso del sistema de archivos SPIFFS
    {
        Serial.println("Card Mount Failed");
        return;
    }

    if (SPIFFS.exists("/data.csv"))
        SPIFFS.remove("/data.csv"); // eliminar archivo data.csv al iniciar el sistema

    // if (SPIFFS.exists(fileData))
    //     SPIFFS.remove(fileData); // eliminar archivo data.csv al iniciar el sistema

    // if (SPIFFS.exists("/read-cursor.tx"))
    //     SPIFFS.remove("/read-cursor.tx"); // eliminar archivo data.csv al iniciar el sistema

    if (!SPIFFS.exists("/general_config.txt"))
    {
        File file_gen = SPIFFS.open("/general_config.txt", FILE_WRITE);
        file_gen.print("NW:Helpmedica^                                       ^\n");
        file_gen.print("CW:9001295137^                                       ^\n");
        file_gen.print("NC:rAKiQgBfJ1x5HFjpLZck^                             ^\n");
        file_gen.print("NA:T98DK7KlZsFzAYhU5uh1^                             ^\n");
        file_gen.close();
        Serial.println("file general create");
    }

    if (!SPIFFS.exists("/frequency.txt"))
    {
        File file_sen = SPIFFS.open("/frequency.txt", FILE_WRITE);
        file_sen.print("10^^^^^\n"); // 1 minuto por defecto
        file_sen.close();
        Serial.println("file frequency create");
    }

    for (int i = 0; i < keyValues::TYPE_COUNT; i++)
    {
        String fileName = "/limit_" + String(keyValueToString(static_cast<keyValues::Type>(i))) + ".txt";
        if (!SPIFFS.exists(fileName))
        {
            File limitFile = SPIFFS.open(fileName, FILE_WRITE);
            limitFile.printf("L:%d,%d^^^^^^^^^^^\n", alarmLimits.at(static_cast<keyValues::Type>(i)).min, alarmLimits.at(static_cast<keyValues::Type>(i)).max);
            limitFile.close();
            Serial.println("file " + fileName + " create");
        }
    }

    for (int i = 0; i < keyValues::TYPE_COUNT; i++)
    {
        String fileName = "/cal_" + String(keyValueToString(static_cast<keyValues::Type>(i))) + ".txt";
        if (!SPIFFS.exists(fileName))
        {
            File calFile = SPIFFS.open(fileName, FILE_WRITE);
            calFile.printf("50.8,68.8,88.0,50.8,68.8,88.0^^^^^^^^^^^^^^^^^^^^^\n");
            calFile.close();
            Serial.println("file " + fileName + " create");
        }
    }

    if (!SPIFFS.exists("/pcb_name.txt"))
    {
        File file_name = SPIFFS.open("/pcb_name.txt", FILE_WRITE);
        file_name.print("Biológicos Factores de Coagulación^             ^^^\n"); // 50 caracteres de contenido (incluyendo nueva línea)
        file_name.close();
        Serial.println("file pcb name create");
    }
}

void files_read()
{
    _funciones_spiffs.Leer_Spiffs_name_card(name_card);
    _funciones_spiffs.Leer_Spiffs_sen(&publish_freq_num);

    // Inicializar el timer con la frecuencia de publicación configurada
    // Convertir milisegundos a microsegundos para el timer
    setupPublishTimer(publish_freq_num * 1000);

    _funciones_spiffs.Leer_Spiffs_gen(datos_recibidos_gen.config_nombre_wifi, datos_recibidos_gen.config_clave_wifi, datos_recibidos_gen.configcompany_name, datos_recibidos_gen.configarea_name);

    _funciones_spiffs.load_all_alarm_limits();

    company_name = datos_recibidos_gen.configcompany_name;
    area_name = datos_recibidos_gen.configarea_name;

    // topic_rtc = company_name;
    topic_tro = "empresa/" + company_name + "/area/" + area_name + "/sensor/" + _manager.id().c_str() + "/trouble/down";
    topic_pub = "empresa/" + company_name + "/area/" + area_name + "/sensor/" + _manager.id().c_str() + "/down";
    topic_sub = "empresa/" + company_name + "/area/" + area_name + "/sensor/" + _manager.id().c_str() + "/up";
}

// Callback que se ejecutará cuando el timer se dispare
void IRAM_ATTR onPublishTimer(void *arg)
{
    // timer_triggered = true;
    completed_time = true; // Indica que se debe publicar los datos de sensores
}

// Función para inicializar el timer con el tiempo de publicación
void setupPublishTimer(uint64_t interval_us)
{
    // Si el timer ya existe, elimínalo primero
    if (publish_timer != NULL)
    {
        esp_timer_stop(publish_timer);
        esp_timer_delete(publish_timer);
        publish_timer = NULL;
    }

    // Configuración del timer
    esp_timer_create_args_t timer_config = {
        .callback = &onPublishTimer,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "publish_timer"};

    // Crear el timer
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &publish_timer));

    // Iniciar el timer con el intervalo especificado (en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(publish_timer, interval_us));

    // Serial.printf("Timer configurado para %llu microsegundos\n", interval_us);
}
