/*Librería con funciones del archivo "Interrupción de botones" ubicado en la ruta: C:\Users\Usuario\Desktop\Pruebas Andrés\Firmware\Integración con edición de tópicos SPIFFS\Inte...SPIFFS 1
Función almacenar_spiffs agregada
Función leer_spiffs agregada
Uso de punteros para pasar valores por referencia a los parámetros de las funciones
*/

// tipos de sensores
const char S_TEMP_HUM1[10] = "1TH1";                 // referencia del sensor HDC1080
const char S_GIROSCOPIO1[10] = "2GAM1";              // referencia del sensor MPU9250
const char S_CORRIENTE1[10] = "3CV1";                // referencia del sensor SCT013
const char S_PRESION1[10] = "4P1";                   // referencia del sensor HK1100C
const char S_CALIDAD_DE_AIRE1[10] = "5A1";           // referencia del sensor MQ135
const char S_INCENDIOS1[10] = "6I1";                 // referencia del sensor MQ2
const char SWITCH1[5] = "7SW1", SWITCH2[5] = "7SW2"; // actuadores o switches
const char S_TEMP_1[10] = "13T1";                    // referencia del sensor DS18B20
                                                     // const char RFID1[10] = "ID";                       //referencia RC522
                                                     // const char LECTOR_QR1[10] = "QR";                  //referencia

// const unsigned int BYTES_PER_ROW = 31; // número de caracteres o bytes que hay en cada fila del registro de datos, depende de cuantos datos queramos guardar, sacando el caracter nulo
// const unsigned int BYTES_STORE = 36449; // número de bytes destinados para guardar información de sensores, dejando 6000 para información de configuración

#include "funciones_spiffs.h"

static char timestamp_main[22];
static bool *p_permiso_json;
static bool estado_internet_spiffs = false;

static unsigned int cont_registro = 0;

// punteros char array para modificar variable del archivo main desde la librería
char *p_config_nombre_wifi, *p_config_clave_wifi, *p_config_nombre_clinica, *p_config_nombre_area; // punteros para configuración general
char publish_freq_char[8];                                                                         // configuración de sensores cada 60 segundos como mínimo (recibe valores en minutos)

char nivel_min[10], nivel_max[10]; // variables de niveles de alarma de sensores

// bool flag_nwifi, flag_cwifi, flag_nclinica, flag_narea;
// bool flag_fpub
bool flag_Amin, flag_Amax;

void funciones_spiffs::process_file(JsonDocument json)
{
    if (json["general_config"])
        save_general_settings(json);
    else if (json["frequency"])
        save_frequency(json);
    else if (json["update"])
        update_device(json);
    else if (json["limites"])
        save_alarm_limits(json);
    else if (json["pcb_name"])
        save_device_name(json);
    else if (json["calibration"])
        save_calibration_points(json);
    else
        Serial.println("Comando incorrecto");
}

// correcto funcionamientos
void funciones_spiffs::save_general_settings(JsonDocument json)
{
    // Serial.println("Configurando Sensores");

    // Obtiene el arreglo "general_config" del JSON
    JsonArray generalConfig = json["general_config"];

    // Obtiene el primer objeto del arreglo "general_config"
    JsonObject config = generalConfig[0];

    // Obtiene los valores de "ssid", "password", "company_id" y "fs_id"
    const char *ssid = config["ssid"];
    const char *password = config["password"];
    const char *companyId = config["company_id"];
    const char *fsId = config["fs_id"];

    File file_gen_save = SPIFFS.open("/general_config.txt", "r+");

    if (strcmp(ssid, "*") != 0)
    {
        file_gen_save.seek(3, SeekSet);
        file_gen_save.printf("%s^", ssid);
    }

    if (strcmp(password, "*") != 0)
    {
        file_gen_save.seek(58, SeekSet);
        file_gen_save.printf("%s^", password);
    }

    if (strcmp(companyId, "*") != 0)
    {
        file_gen_save.seek(113, SeekSet);
        file_gen_save.printf("%s^", companyId);
    }

    if (strcmp(fsId, "*") != 0)
    {
        file_gen_save.seek(168, SeekSet);
        file_gen_save.printf("%s^", fsId);
    }

    file_gen_save.close();

    // Serial.println("Configuracion general completada");

    ESP.restart();
}

// correcto funcionamiento
void funciones_spiffs::save_frequency(JsonDocument json)
{
    // Serial.println("Configurando frecuencia");

    // Obtiene el arreglo "general_config" del JSON
    JsonArray generalConfig = json["frequency"];

    // Obtiene el primer objeto del arreglo "general_config"
    JsonObject config = generalConfig[0];

    // Obtiene los valores de "ssid", "password", "company_id" y "fs_id"
    const char *publish_freq = config["value"];

    // strcpy(publish_freq, json["value"]);

    // Serial.println(publish_freq);

    File file_sen_save = SPIFFS.open("/frequency.txt", "r+"); // crea un archivo de texto llamado credenciales, que almacenará todos los datos que reciba de la aplicación. la "r+" permite leer y escribir datos en posiciones específicas mediante el método seek()
    // file_sen_save.seek(3, SeekSet);
    file_sen_save.printf("%s^", publish_freq);
    file_sen_save.close();

    flag_callback_broker = true;
}

// correcto funcionamiento
void funciones_spiffs::save_alarm_limits(JsonDocument json)
{
    // Verificar si el JSON contiene el arreglo "limites"
    if (!json["limites"].is<JsonArray>())
        return;

    // Obtener el arreglo "limites" del JSON
    JsonArray limites = json["limites"].as<JsonArray>();

    // Iterar sobre cada objeto en el arreglo "limites"
    for (JsonObject limite : limites)
    {
        const char *type = limite["type"];
        int min = limite["min"];
        int max = limite["max"];

        // Crear el nombre del archivo basado en el tipo
        String fileName = String("/limit_") + type + ".txt";

        // Verificar si el archivo ya existe
        if (SPIFFS.exists(fileName))
        {
            // Serial.println("El archivo ya existe, se sobrescribirá");

            // Guardar cada límite en un archivo en SPIFFS
            File file = SPIFFS.open(fileName, FILE_WRITE);
            if (!file)
                continue;

            file.printf("L:%d,%d^", min, max);
            file.close();
        }
    }

    // Serial.println("Límites de alarma guardados correctamente");

    flag_callback_broker = true;

    // Serial.println("final guardado");
}

// correcto funcionamiento
void funciones_spiffs::update_device(JsonDocument json)
{
    // Serial.println("Update Device");

    // Obtiene el arreglo "general_config" del JSON
    JsonArray update_firmware = json["update"];

    // Obtiene el primer objeto del arreglo "general_config"
    JsonObject update = update_firmware[0];

    // Obtiene los valores de "update"
    String _location = update["location"];

    location = _location;

    // Serial.println(location);
    firmware = true;
}

// correcto funcionamiento
void funciones_spiffs::save_device_name(JsonDocument json)
{
    // Obtiene el arreglo "pcb_name" del JSON
    JsonArray modify_pcb_name = json["pcb_name"];

    // Obtiene el primer objeto del arreglo "pcb_name"
    JsonObject pcb_name = modify_pcb_name[0];

    const char *name = pcb_name["name"];

    // Serial.println(name);

    File file_name_save = SPIFFS.open("/pcb_name.txt", "r+");
    file_name_save.printf("%s^", name);
    file_name_save.close();

    // Serial.println("Nombre del dispositivo actualizado");

    flag_callback_broker = true;
}

// correcto funcionamiento
void funciones_spiffs::save_calibration_points(JsonDocument json)
{
    char name_file[30] = "*";

    // Obtiene el arreglo "calibration points" del JSON
    JsonArray calibration_points = json["calibration"];

    // Obtiene el primer objeto del arreglo "calibration points"
    JsonObject calibration = calibration_points[0];

    if (calibration["type"])
    {
        // Serial.println("type encontrado");

        // const char *type = calibration["type"];
        String type = calibration["type"];
        const float value1 = calibration["value1"];
        const float value2 = calibration["value2"];
        const float value3 = calibration["value3"];
        const float value4 = calibration["value4"];
        const float value5 = calibration["value5"];
        const float value6 = calibration["value6"];

        // quitar espacios en blanco
        type.trim();
        // type.toLowerCase();

        strcpy(name_file, "/cal_");
        strcat(name_file, type.c_str());
        strcat(name_file, ".txt");

        // Serial.println(name_file);

        if (SPIFFS.exists(name_file)) // Verificar si hay un archivo con ese nombre
        {
            // Serial.println("Archivo encontrado, guardar");
            File file_cal_save = SPIFFS.open(name_file, "r+");
            file_cal_save.printf("%.1f,", value1); // Guardar máximo 1 decimal flotante
            file_cal_save.printf("%.1f,", value2);
            file_cal_save.printf("%.1f,", value3);
            file_cal_save.printf("%.1f,", value4);
            file_cal_save.printf("%.1f,", value5);
            file_cal_save.printf("%.1f^", value6);
            file_cal_save.close();

            // ESP.restart();
            flag_callback_broker = true;
        }
        // else
        //     Serial.println("El archivo no existe");
    }
}

bool funciones_spiffs::read_data(DS18B20Saved &_DS18B20Saved)
{
    Serial.println("Reading data");
    String data;
    if (SPIFFS.exists(fileData))
    {
        std::vector<String> dataSaved = getData();
        if (!dataSaved.empty())
        {
            data = dataSaved.front();
            Serial.println(data);
            sscanf(data.c_str(), "%21[^,],%9[^,],%9[^,],%2",
                   _DS18B20Saved._DS18B20Data.time,
                   &_DS18B20Saved._DS18B20Data.temperature,
                   &_DS18B20Saved.energy);
            // if (_DS18B20Saved.energy > 1)
            //     _DS18B20Saved.energy = 1;
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

bool funciones_spiffs::saveAlertWithTimestamp(const char *timestamp, const char *type)
{
    String fileName = "/alert_" + String(type) + ".txt";
    // Serial.println(fileName);
    File file = SPIFFS.open(fileName, FILE_READ);
    if (!file)
        return false;

    String lastTimestamp;
    while (file.available())
    {
        lastTimestamp = file.readStringUntil('^');
    }
    file.close();

    if (lastTimestamp.length() > 0)
    {
        struct tm lastTime;
        strptime(lastTimestamp.c_str(), "%Y-%m-%d %H:%M:%S", &lastTime);
        time_t lastTimeEpoch = mktime(&lastTime);

        struct tm currentTime;
        strptime(timestamp, "%Y-%m-%d %H:%M:%S", &currentTime);
        time_t currentTimeEpoch = mktime(&currentTime);

        double secondsDiff = difftime(currentTimeEpoch, lastTimeEpoch);
        if (secondsDiff < 3600)
            return false;
    }

    file = SPIFFS.open(fileName, FILE_WRITE);
    if (!file)
        return false;

    file.printf("%s^", timestamp);
    file.close();
    // Serial.println("Alerta guardada");

    return true;
}

void funciones_spiffs::save_data(String _timestamp, const DS18B20Data &_DS18B20Data, int &_energy)
{
    if (_timestamp.startsWith("1970") || _timestamp.startsWith("*"))
        return;

#define BYTES_STORE 1438481 // 1570864

    if (SPIFFS.totalBytes() - SPIFFS.usedBytes() >= BYTES_STORE)
        return;

    // if (_DS18B20Data.temperature > 125 || _DS18B20Data.temperature <= -55) // si la temperatura es mayor a 100 o menor o igual a 0, se asigna un valor de NAN
    //     return;

    // Serial.println("Saving data");
    // String timestamp = _timestamp;
    // float temperature = _DS18B20Data.temperature;
    // float humidity = _DS18B20Data->humidity;
    // Serial.println(timestamp);
    // Serial.println(temperature);
    // Serial.println(humidity);
    Serial.println("Guardando datos");
    Serial.println(_timestamp);
    Serial.println(_DS18B20Data.temperature);
    Serial.println(_energy);
    File file = SPIFFS.open(fileData, FILE_APPEND);
    if (!file)
        return;
    file.printf("%s,%s,%d\n", _timestamp.c_str(), _DS18B20Data.temperature, _energy);
    file.close();
}

void funciones_spiffs::delete_first_data()
{
    // Serial.println("Deleting first data");
    std::vector<String> dataSaved = getData();
    if (dataSaved.empty())
    {
        SPIFFS.remove(fileData);
        return;
    }

    dataSaved.erase(dataSaved.begin());

    File file = SPIFFS.open(fileData, FILE_WRITE);
    if (!file)
        return;
    for (const auto &data : dataSaved)
        file.println(data);
    file.close();
}

std::vector<String> funciones_spiffs::getData()
{
    std::vector<String> dataSaved;

    File file = SPIFFS.open(fileData, FILE_READ);
    if (!file)
        return dataSaved;

    while (file.available())
    {
        String data = file.readStringUntil('\n');
        data.trim();
        if (data.length() > 0)
        {
            dataSaved.push_back(data);
        }
    }

    file.close();
    return dataSaved;
}

// LEER TODOS LOS SPIFFS
void funciones_spiffs::Leer_Spiffs_gen(char *p_nombre_wifi, char *p_clave_wifi, char *p_nombre_clinica, char *p_nombre_area)
{
    // Serial.println("void leer_spiffs_gen");

    char contenido[240] = "*";
    unsigned int i = 0;

    File file_gen_read = SPIFFS.open("/general_config.txt"); // si no hay nada en la memoria o no hay ningún archivo con este nombre, el esp32 se reinicia. averiguar primero si el archivo existe
    if (!file_gen_read)
        return;

    bool finish = false;
    int rows = 0;
    while (file_gen_read.available() && finish == false)
    {
        contenido[i] = file_gen_read.read();
        i++;

        if (contenido[i] == ':')
            rows++;

        if (rows == 4)
            if (contenido[i] == '^')
                finish = true;
    }
    file_gen_read.close();

    // Serial.println(contenido);

    strtok(contenido, ":");
    strcpy(p_nombre_wifi, strtok(NULL, "^"));
    strtok(NULL, ":");
    strcpy(p_clave_wifi, strtok(NULL, "^"));
    strtok(NULL, ":");
    strcpy(p_nombre_clinica, strtok(NULL, "^"));
    strtok(NULL, ":");
    strcpy(p_nombre_area, strtok(NULL, "^"));

    // Serial.printf("Nombre Wifi  => %s\nClave Wifi  => %s\nNombre Clinica  => %s\nNombre_Area  => %s\n", p_nombre_wifi, p_clave_wifi, p_nombre_clinica, p_nombre_area);
}

void funciones_spiffs::Leer_Spiffs_sen(float *p_publish_freq_num)
{
    // Serial.println("void leyendo frecuencia");

    char contenido[10] = "*"; // variable que almacena contenido de archivo de texto
    int i = 0;
    // publish_freq_num = 60000; //publicar datos de sensores cada 60 segundos como mínimo (recibe valores en minutos)

    File file_sen_read = SPIFFS.open("/frequency.txt"); // si no hay nada en la memoria o no hay ningún archivo con este nombre, el esp32 se reinicia. averiguar primero si el archivo existe
    if (!file_sen_read)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    bool finish = false;
    while (file_sen_read.available() && finish == false)
    {
        contenido[i] = file_sen_read.read();
        contenido[i] == '^' ? finish = true : i++;
    }
    file_sen_read.close();

    strcpy(publish_freq_char, strtok(contenido, "^"));

    // CONVERSIÓN DE CARACTERES A NÚMEROS ENTEROS
    *p_publish_freq_num = atof(publish_freq_char);             // frecuencia de publicación de datos de sensores
    *p_publish_freq_num = *p_publish_freq_num * 60.0 * 1000.0; // conversión de valor recibido de minutos a milisegundos

    // Serial.printf("Frecuencia de publicación en milisegundos: %f\n", *p_publish_freq_num);
}

void funciones_spiffs::readCalibrationPoints(const char *type, float values[6])
{
    char name_file[30] = "*";

    // Serial.println("Reading calibration points");
    // Serial.println(type);

    strcpy(name_file, "/cal_");
    strcat(name_file, type);
    strcat(name_file, ".txt");

    // Serial.println(name_file);

    File file_cal_read = SPIFFS.open(name_file); // si no hay nada en la memoria o no hay ningún archivo con este nombre, el esp32 se reinicia. averiguar primero si el archivo existe
    if (!file_cal_read)
    {
        // Serial.println("Failed to open file for reading");
        return;
    }

    char contenido[50] = "*";
    unsigned int i = 0;

    bool finish = false;
    while (file_cal_read.available() && finish == false)
    {
        contenido[i] = file_cal_read.read();
        contenido[i] == '^' ? finish = true : i++;
    }
    file_cal_read.close();

    // Serial.println(contenido);
    // char *token = strtok(contenido, ",");
    values[0] = atof(strtok(contenido, ","));
    for (int i = 1; i < 5; i++)
        values[i] = atof(strtok(NULL, ","));
    values[5] = atof(strtok(NULL, "^"));
}

// void funciones_spiffs::read_spiffs_alarm(char *file_name, char *p_nivel_minimo, char *p_nivel_maximo) // el parámetro "tipo_de_sensor" hace referencia al número que identifica cada tipo de sensor. este puede encontrarse en el documento llamado "tramas de dispositivos"

void funciones_spiffs::load_alarm_limits(const char *fileName, int &min, int &max)
{
    File file = SPIFFS.open(fileName, FILE_READ);
    if (!file)
    {
        // Serial.println("Error al abrir el archivo para leer");
        return;
    }

    // int min, max;
    file.find("L:");
    min = file.parseInt();
    file.find(",");
    max = file.parseInt();
    file.close();
}

void funciones_spiffs::load_all_alarm_limits()
{
    // Serial.println("Cargando todos los límites de alarma");
    int min = 0, max = 0;

    for (int i = 0; i < keyValues::TYPE_COUNT; i++)
    {
        String fileName = "/limit_" + String(keyValueToString(static_cast<keyValues::Type>(i))) + ".txt";
        load_alarm_limits(fileName.c_str(), min, max);

        // if (min != 0 && max != 0)
        alarmLimits[static_cast<keyValues::Type>(i)].min = min, alarmLimits[static_cast<keyValues::Type>(i)].max = max;
        // else
        // Serial.println("Error al cargar los límites de alarma");
    }
}

void funciones_spiffs::Leer_Spiffs_cal(char *p_value_1, char *p_value_2, char *p_value_3, char *m_value_1, char *m_value_2, char *m_value_3, int campo_del_sensor)
{
    // Serial.println("void leer_spiffs_gen");

    char contenido[100] = "*";
    unsigned int i = 0;

    File file_cal_read = SPIFFS.open("/cal_temperatura.txt"); // si no hay nada en la memoria o no hay ningún archivo con este nombre, el esp32 se reinicia. averiguar primero si el archivo existe
    if (!file_cal_read)
    {
        // Serial.println("Failed to open file for reading");
        return;
    }

    /*if (campo_del_sensor == 1)
        file_cal_read.seek(0, SeekSet);*/

    bool finish = false;
    while (file_cal_read.available() && finish == false)
    {
        contenido[i] = file_cal_read.read();
        contenido[i] == '^' ? finish = true : i++;
        // Serial.write(file_gen_read.read());
    }

    // Serial.println(contenido);
    file_cal_read.close();

    // Serial.println(contenido);
    //  strtok(contenido, ":");
    strcpy(p_value_1, strtok(contenido, ","));
    strcpy(p_value_2, strtok(NULL, ","));
    strcpy(p_value_3, strtok(NULL, ","));
    strcpy(m_value_1, strtok(NULL, ","));
    strcpy(m_value_2, strtok(NULL, ","));
    strcpy(m_value_3, strtok(NULL, "^"));
}

void funciones_spiffs::Leer_Spiffs_name_card(char *name_card)
{
    // Serial.println("Leer nombre de la tarjeta");

    char contenido[60] = "*";
    unsigned int i = 0;

    File file_name_read = SPIFFS.open("/pcb_name.txt"); // si no hay nada en la memoria o no hay ningún archivo con este nombre, el esp32 se reinicia. averiguar primero si el archivo existe
    if (!file_name_read)
    {
        // Serial.println("Failed to open file for reading");
        return;
    }

    // file_name_read.seek(0, SeekSet);
    bool finish = false;
    while (file_name_read.available() && finish == false)
    {
        contenido[i] = file_name_read.read();
        contenido[i] == '^' ? finish = true : i++;

        // Serial.write(file_gen_read.read());
    }
    // Serial.println(contenido);
    file_name_read.close();

    // Serial.println(contenido);
    //  strtok(contenido, ":");
    strcpy(name_card, strtok(contenido, "^"));
}
