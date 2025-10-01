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

// Función para monitoreo de memoria y prevención de stack overflow
static void checkMemoryStatus()
{
    size_t freeHeap = ESP.getFreeHeap();
    size_t minFreeHeap = ESP.getMinFreeHeap();

    if (freeHeap < 8192)
    { // Menos de 8KB libres
        Serial.println("WARNING: Memoria baja - " + String(freeHeap) + " bytes libres");
    }

    if (freeHeap < 4096)
    { // Menos de 4KB libres - crítico
        Serial.println("CRITICAL: Memoria críticamente baja - forzando garbage collection");
        delay(100); // Dar tiempo al sistema
    }
}

// Función segura para copiar strings con verificación de límites
static bool safe_strcpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0)
        return false;

    size_t src_len = strlen(src);
    if (src_len >= dest_size)
    {
        Serial.println("Error: String demasiado largo para el buffer");
        return false;
    }

    strcpy(dest, src);
    return true;
}

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

    Serial.println("Configuracion general completada");

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

    // Actualizar la frecuencia de publicación y reiniciar el timer
    extern float publish_freq_num;
    extern void setupPublishTimer(uint64_t interval_us);

    // Convertir el string a un valor numérico
    publish_freq_num = atof(publish_freq);

    // Configurar el timer con la nueva frecuencia (convertir de milisegundos a microsegundos)
    setupPublishTimer(publish_freq_num * 1000);
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
    const int MAX_FILENAME = 35; // Aumentar límite para mayor seguridad
    char name_file[MAX_FILENAME];
    memset(name_file, 0, MAX_FILENAME); // Inicializar buffer

    // Obtiene el arreglo "calibration points" del JSON
    JsonArray calibration_points = json["calibration"];

    // Obtiene el primer objeto del arreglo "calibration points"
    JsonObject calibration = calibration_points[0];

    if (calibration["type"])
    {
        // Serial.println("type encontrado");

        String type = calibration["type"];
        const float value1 = calibration["value1"];
        const float value2 = calibration["value2"];
        const float value3 = calibration["value3"];
        const float value4 = calibration["value4"];
        const float value5 = calibration["value5"];
        const float value6 = calibration["value6"];

        // quitar espacios en blanco
        type.trim();

        // Verificar longitud del tipo para evitar overflow
        if (type.length() > 20)
        {
            Serial.println("Error: Tipo de sensor demasiado largo");
            return;
        }

        // Usar snprintf para mayor seguridad
        int result = snprintf(name_file, MAX_FILENAME, "/cal_%s.txt", type.c_str());
        if (result >= MAX_FILENAME || result < 0)
        {
            Serial.println("Error: Nombre de archivo demasiado largo");
            return;
        }

        // Serial.println(name_file);

        if (SPIFFS.exists(name_file)) // Verificar si hay un archivo con ese nombre
        {
            // Serial.println("Archivo encontrado, guardar");
            File file_cal_save = SPIFFS.open(name_file, "r+");
            if (!file_cal_save)
            {
                Serial.println("Error: No se pudo abrir archivo de calibración");
                return;
            }

            // Usar fprintf con verificación de errores
            if (fprintf(file_cal_save.getWriteError() ? stderr : (FILE *)&file_cal_save, "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f^", value1, value2, value3, value4, value5, value6) < 0)
            {
                Serial.println("Error: No se pudo escribir datos de calibración");
            }
            file_cal_save.close();

            flag_callback_broker = true;
        }
        else
        {
            Serial.println("Warning: El archivo de calibración no existe");
        }
    }
}

// bool funciones_spiffs::read_data(DS18B20Saved &_DS18B20Saved)
// {
//     Serial.println("Reading data");
//     String data;
//     if (SPIFFS.exists(fileData))
//     {
//         std::vector<String> dataSaved = getData();
//         if (!dataSaved.empty())
//         {
//             data = dataSaved.front();
//             Serial.println(data);
//             sscanf(data.c_str(), "%21[^,],%9[^,],%9[^,],%2",
//                    _DS18B20Saved._DS18B20Data.time,
//                    &_DS18B20Saved._DS18B20Data.temperature,
//                    &_DS18B20Saved.energy);
//             // if (_DS18B20Saved.energy > 1)
//             //     _DS18B20Saved.energy = 1;
//             return true;
//         }
//         else
//             return false;
//     }
//     else
//         return false;
// }

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

// void funciones_spiffs::save_data(String _timestamp, const DS18B20Data &_DS18B20Data, int &_energy)
// {
//     // Verificar memoria antes de procesar
//     checkMemoryStatus();

//     if (_timestamp.startsWith("1970") || _timestamp.startsWith("*"))
//     {
//         Serial.println("Error: Timestamp inválido");
//         return;
//     }

// #define BYTES_STORE 1438481 // 1570864

//     size_t freeSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();
//     if (freeSpace < BYTES_STORE)
//     {
//         Serial.println("Error: Espacio insuficiente en SPIFFS");
//         return;
//     }

//     // Validar datos de temperatura (convertir string a float para validación)
//     float temp_value = atof(_DS18B20Data.temperature);
//     if (temp_value > 125 || temp_value <= -55)
//     {
//         Serial.println("Error: Temperatura fuera de rango válido: " + String(temp_value));
//         return;
//     }

//     // Validar que el string de temperatura no esté vacío o sea inválido
//     if (strcmp(_DS18B20Data.temperature, "*") == 0 || strlen(_DS18B20Data.temperature) == 0)
//     {
//         Serial.println("Error: Datos de temperatura inválidos");
//         return;
//     }

//     // Validar datos de energía
//     if (_energy < 0 || _energy > 100)
//     {
//         Serial.println("Warning: Valor de energía sospechoso: " + String(_energy));
//     }

//     Serial.println("Guardando datos");
//     Serial.println(_timestamp);
//     Serial.println(_DS18B20Data.temperature);
//     Serial.println(_energy);

//     File file = SPIFFS.open(fileData, FILE_APPEND);
//     if (!file)
//     {
//         Serial.println("Error: No se pudo abrir archivo para escritura");
//         return;
//     }

//     // Usar fprintf con verificación de errores
//     int bytes_written = file.printf("%s,%s,%d\n", _timestamp.c_str(), _DS18B20Data.temperature, _energy);
//     if (bytes_written <= 0)
//     {
//         Serial.println("Error: No se pudieron escribir los datos");
//     }

//     file.close();
// }

// void funciones_spiffs::delete_first_data()
// {
//     // Verificar memoria antes de procesar
//     checkMemoryStatus();

//     Serial.println("Eliminando primer registro de datos");

//     std::vector<String> dataSaved = getData();
//     if (dataSaved.empty())
//     {
//         Serial.println("No hay datos para eliminar, removiendo archivo");
//         SPIFFS.remove(fileData);
//         return;
//     }

//     if (dataSaved.size() <= 1)
//     {
//         Serial.println("Solo hay un registro, removiendo archivo");
//         SPIFFS.remove(fileData);
//         return;
//     }

//     // Verificar memoria antes de continuar
//     if (ESP.getFreeHeap() < 8192)
//     {
//         Serial.println("Error: Memoria insuficiente para operación de eliminación");
//         return;
//     }

//     dataSaved.erase(dataSaved.begin());

//     File file = SPIFFS.open(fileData, FILE_WRITE);
//     if (!file)
//     {
//         Serial.println("Error: No se pudo abrir archivo para escritura");
//         return;
//     }

//     size_t records_written = 0;
//     for (const auto &data : dataSaved)
//     {
//         if (file.println(data) <= 0)
//         {
//             Serial.println("Error: No se pudo escribir registro");
//             break;
//         }
//         records_written++;

//         // Verificar memoria periódicamente durante la escritura
//         if (records_written % 50 == 0 && ESP.getFreeHeap() < 4096)
//         {
//             Serial.println("Warning: Memoria baja durante escritura, deteniendo...");
//             break;
//         }
//     }

//     file.close();
//     Serial.println("Registros escritos: " + String(records_written));
// }

// std::vector<String> funciones_spiffs::getData()
// {
//     int i = 0;
//     std::vector<String> dataSaved;
//     const int MAX_RECORDS = 512; // Reducir límite para evitar overflow de memoria
//     const int MAX_LINE_LENGTH = 100; // Limitar longitud de línea

//     File file = SPIFFS.open(fileData, FILE_READ);
//     if (!file)
//     {
//         Serial.println("Error: No se pudo abrir el archivo para lectura");
//         return dataSaved;
//     }

//     // Verificar memoria disponible antes de procesar
//     size_t freeHeap = ESP.getFreeHeap();
//     if (freeHeap < 10000) // Mínimo 10KB libres
//     {
//         Serial.println("Error: Memoria insuficiente para procesar datos");
//         file.close();
//         return dataSaved;
//     }

//     // Reservar memoria para evitar realocaciones frecuentes
//     dataSaved.reserve(MAX_RECORDS);

//     while (file.available() && i < MAX_RECORDS)
//     {
//         String data = file.readStringUntil('\n');
//         data.trim();

//         // Verificar longitud de línea para evitar problemas de memoria
//         if (data.length() > MAX_LINE_LENGTH)
//         {
//             Serial.println("Warning: Línea demasiado larga, omitiendo...");
//             i++;
//             continue;
//         }

//         Serial.print("Registro ");
//         Serial.println(i);
//         i++;

//         if (data.length() > 0)
//         {
//             // Verificar memoria antes de agregar cada elemento
//             if (ESP.getFreeHeap() < 5000)
//             {
//                 Serial.println("Warning: Memoria baja, deteniendo lectura");
//                 break;
//             }
//             dataSaved.push_back(data);
//         }
//     }

//     file.close();

//     Serial.print("Total registros leídos: ");
//     Serial.println(dataSaved.size());
//     Serial.print("Memoria libre después de lectura: ");
//     Serial.println(ESP.getFreeHeap());
//     return dataSaved;
// }

void funciones_spiffs::save_sensor_data(String _timestamp, const DS18B20Data &data, char energy)
{
    // Verificar memoria antes de procesar
    checkMemoryStatus();
    
    if (_timestamp.startsWith("1970") || _timestamp.startsWith("*") || _timestamp.length() == 0)
    {
        Serial.println("Error: Timestamp inválido");
        return;
    }

    // Validar datos de temperatura
    if (strcmp(data.temperature, "*") == 0 || strlen(data.temperature) == 0)
    {
        Serial.println("Error: Datos de temperatura inválidos");
        return;
    }

    Serial.println("Guardando registro de sensores");

    const int BUFFER_SIZE = 40; // Aumentar buffer para mayor seguridad
    char guardar_fila[BUFFER_SIZE];
    memset(guardar_fila, 0, BUFFER_SIZE);
    
    unsigned int longitud_array = 0;
    char array_de_relleno[BYTES_POR_FILA];
    memset(array_de_relleno, 0, sizeof(array_de_relleno));
    unsigned int num_caracteres_a_completar = 0;
    unsigned int numero_filas_usadas = 0;
    unsigned long posicion_del_cursor = 0;

    // Incrementar contador de registros de forma segura
    if (cont_registro < UINT32_MAX - 1)
        cont_registro++;
    else
        cont_registro = 0; // Reset si se alcanza el máximo
    
    // Abrir archivo en modo append
    File file_registro_save = SPIFFS.open(fileData, "a");
    if (!file_registro_save)
    {
        Serial.println("Error: No se pudo abrir archivo para escritura");
        return;
    }
    
    posicion_del_cursor = file_registro_save.position();
    numero_filas_usadas = posicion_del_cursor / BYTES_POR_FILA;

    if ((numero_filas_usadas + 1) <= FILAS_REGISTRO_TEMPERATURA)
    {
        // Convertir energy a string de forma segura
        String energyStr = String((int)energy);
        
        // Verificar longitudes antes de concatenar
        size_t timestamp_len = _timestamp.length();
        size_t temp_len = strlen(data.temperature);
        size_t energy_len = energyStr.length();
        
        // +2 para las comas, verificar que todo quepa
        if (timestamp_len + temp_len + energy_len + 2 > BUFFER_SIZE - 1)
        {
            Serial.println("Error: Datos demasiado largos para el buffer");
            file_registro_save.close();
            return;
        }
        
        // Construir la cadena de forma segura
        int result = snprintf(guardar_fila, BUFFER_SIZE, "%s,%s,%s", 
                             _timestamp.c_str(), data.temperature, energyStr.c_str());
        
        if (result < 0 || result >= BUFFER_SIZE)
        {
            Serial.println("Error: Error al formatear la cadena");
            file_registro_save.close();
            return;
        }

        Serial.printf("GUARDAR FILA INICIAL: %s\n", guardar_fila);
        longitud_array = strlen(guardar_fila);
        Serial.printf("LONGITUD FILA: %d\n", longitud_array);

        // Rellenar hasta alcanzar el tamaño requerido
        if (longitud_array <= BYTES_POR_FILA - 2) 
        {
            num_caracteres_a_completar = BYTES_POR_FILA - longitud_array - 1;
            
            // Verificar que no se exceda el buffer
            if (longitud_array + num_caracteres_a_completar + 1 < BUFFER_SIZE)
            {
                // Rellenar con caracteres "^"
                for (unsigned int i = 0; i < num_caracteres_a_completar && i < (sizeof(array_de_relleno) - 1); i++) {
                    array_de_relleno[i] = '^';
                }
                array_de_relleno[num_caracteres_a_completar] = '\0';
                
                // Añadir relleno de forma segura
                strncat(guardar_fila, array_de_relleno, BUFFER_SIZE - strlen(guardar_fila) - 2);
                strncat(guardar_fila, "\n", BUFFER_SIZE - strlen(guardar_fila) - 1);
            }
        }
        else if (longitud_array > BYTES_POR_FILA - 2)
        {
            // Si la longitud excede, truncar y añadir ^ y \n al final
            if (BYTES_POR_FILA - 2 < BUFFER_SIZE && BYTES_POR_FILA - 1 < BUFFER_SIZE)
            {
                guardar_fila[BYTES_POR_FILA - 2] = '^'; 
                guardar_fila[BYTES_POR_FILA - 1] = '\n';
                guardar_fila[BYTES_POR_FILA] = '\0';
            }
        }

        // Escribir la fila al archivo con verificación
        if (file_registro_save.print(guardar_fila) <= 0)
        {
            Serial.println("Error: No se pudo escribir al archivo");
        }
        else
        {
            Serial.printf("GUARDAR FILA FINAL: %s\n", guardar_fila);
        }
    }
    else
    {
        Serial.println("límite de almacenamiento de mediciones alcanzado");
    }
    
    file_registro_save.close();
    Serial.println("FIN función registro sensores");
}

void funciones_spiffs::read_sensor_register(bool next, DS18B20Saved &_savedData) // esta tarea es suspendida cuando hay una falla ya sea de wifi, internet o broker
{
    // const unsigned int MUESTRAS_A_ENVIAR = 30;   // nuestras que se envían en un chorro de datos antes de hacer una pausa en el envío
    // unsigned int lectura_sensores_variable = 20; // por defecto el envío de mediciones almacenadas es de 20ms

    static char contenido[BYTES_POR_FILA] = "*";

   
    bool existe_registro_mediciones = false;         // variables para saber si existen archivos de las mediciones y el de la posición del cursor
    unsigned long posicion_del_cursor = 0;           // variable para almacenar el contenido la posición del cursor pasado por parámetro y para guardar la última después de leer cada fila
    unsigned int numero_filas_leidas = 0;            //
    static unsigned int contador_ejecuciones = 0;    // cuenta el número de veces que se envían datos resgistrados al broker. usado para cambiar el tiempo de bloqueo de la tarea
    // static bool permiso_avance_de_fila = true;    // variable para permitir que se lea una nueva fila del registro de sensores si se enviaron los datos de la anterior correctamente en el json

    Serial.println("LEER_REGISTRO_SENSORES");

    existe_registro_mediciones = SPIFFS.exists(fileData); // verificación de que existen los archivos necesarios para leer las mediciones almacenadas en el SPIFFS

    if (existe_registro_mediciones == true)
    {
        Serial.println("Existe el registro de mediciones");

        read_cursor_sensors(&posicion_del_cursor);


        File file_registro_read = SPIFFS.open(fileData, "r+"); // el argumento r+ ubica el cursor al inicio del archivo

        // file_registro_read.seek(fila * BYTES_POR_FILA, SeekSet); //ubicar el cursor
        // posicion_del_cursor = *p_posicion_cursor;
        file_registro_read.seek(posicion_del_cursor, SeekSet);

        Serial.print("posición establecida: ");
        Serial.println(posicion_del_cursor);
        numero_filas_leidas = posicion_del_cursor / BYTES_POR_FILA; // número de filas leídas en el ciclo de lectura de filas anterior

        if (numero_filas_leidas + 1 <= FILAS_REGISTRO_TEMPERATURA) // verificación de disponibilidad de lectura del contenido del archivo. si las filas leídas en el ciclo anterior se le suma la fila que se leerá, debe ser menor o igual al total de filas establecido
        {
            if (next == true)
            {
                contador_ejecuciones++;
                // Serial.print("CONTADOR DE EJECUCIONES: ");
                // Serial.println(contador_ejecuciones);

                memset(contenido, '\0', sizeof(contenido)); // borrar contenido leído del registro

                // Leer con verificación de límites y disponibilidad
                for (int i = 0; i < BYTES_POR_FILA && file_registro_read.available(); i++)
                {
                    char c = file_registro_read.read();
                    if (c != EOF && i < (BYTES_POR_FILA - 1))
                    {
                        contenido[i] = c;
                        // Solo mostrar debug cada 10 caracteres para no saturar
                        if (i % 10 == 0)
                            Serial.print(".");
                    }
                    else
                    {
                        break; // Salir del loop si hay error o EOF
                    }
                }
                contenido[BYTES_POR_FILA - 1] = '\0'; // Asegurar terminación nula
                Serial.println(); // Nueva línea después de los puntos de debug
                
            }
            else
            {
                contador_ejecuciones = 0; // reinicia el contador para que envíe 30 datos la próxima vez que se conecte al broker
                // Serial.println("FALLO AL ENVIAR EL JSON. NO AVANZAR Y SUSPENDER LECTURA DE REGISTRO DE SENSORES");

                posicion_del_cursor = file_registro_read.position(); // adquirir la posición del cursor para guardarla

                save_cursor_position(posicion_del_cursor);
            }

            // Serial.print("disponibilidad de registro: ");
            // Serial.println(file_registro_read.available());

            if (!file_registro_read.available())
            {
                file_registro_read.close();
                if (SPIFFS.exists(fileData) == true && SPIFFS.exists(read_cursor_file_name) == true)
                // if (SPIFFS.exists(fileData) == true)
                {
                    SPIFFS.remove(fileData);
                    SPIFFS.remove(read_cursor_file_name);
                    // file_registro_read.close(); // cerrar el archivo spiffs antes de eliminarlos y suspender tarea
                    //  Serial.println("eliminación de archivos en SPIFFS de registro de mediciones y posicion de cursor");
                    /*Serial.print("estado de eliminación registro-mediciones y cursor_reg_sensores: ");
                    Serial.print(SPIFFS.remove(fileData));          // elimina el archivo de registro si fue leído en su totalidad. arroja true si fue eliminado exitosamente
                    Serial.println(SPIFFS.remove(read_cursor_file_name)); // elimina el archivo donde se almacena la posicion del cursor durante la lectura las mediciones almacenadas en el spiffs

                    Serial.println("Archivos borrados");*/
                    //*p_publica    r_mediciones_spiffs = false;                     //apagar bandera para que no seguir publicando las mediciones guardadas en el SPIFFS debido a que el archivo ha sido eliminado
                    // fin_de_archivo = true; //variable de fin de archivo verdadera, ya se llegó al final
                    // ESP.restart(); //reiniciar el ESP32 después de borrar los archivos de registro de sensores y del cursor
                    // vTaskSuspend(NULL); // la tarea se suspende para que no ejecute el código de abajo. sería equivalente a usar return para salir de funciones
                    // return;
                }
            }

            posicion_del_cursor = file_registro_read.position();
            file_registro_read.close();
            Serial.print("mediciones almacenadas: ");
            Serial.println(contenido);
            Serial.print("posición guardada: ");
            Serial.println(posicion_del_cursor);

            // Almacenar_Cursor_sensores_2(posicion_del_cursor, false); // guardado de la posición del cursor después de leer toda la fila del archivo almacenado en el SPIFFS
            save_cursor_position(posicion_del_cursor); // guardado de la posición del cursor después de leer toda la fila del archivo almacenado en el SPIFFS
            // _funciones_spiffs_.Almacenar_Cursor_sensores(posicion_del_cursor); // guardado de la posición del cursor después de leer toda la fila del archivo almacenado en el SPIFFS

            // char timestamp_saved[22] = "*";
            // char temperature_saved[6] = "*";

            // strcpy(timestamp_saved, strtok(contenido, ",")); // Guardar timestamp
            // strcpy(temperature_saved, strtok(NULL, ",")); // Guardar temperatura


            /*Serial.print("temp save => ");
            Serial.println(temperature_saved);
            Serial.print("timestamp save => ");
            Serial.println(timestamp_saved);*/
            Serial.println("PARSEANDO DATOS DEL REGISTRO DE SENSORES");
            Serial.println(contenido);

            // Verificar que el contenido no esté vacío antes de hacer strtok
            if (strlen(contenido) == 0 || strcmp(contenido, "*") == 0)
            {
                Serial.println("Error: Contenido vacío o inválido");
                // Inicializar con valores por defecto
                strcpy(_savedData._DS18B20Data.time, "*");
                strcpy(_savedData._DS18B20Data.temperature, "*");
                strcpy(_savedData.energy, "*");
                return;
            }

            // Hacer una copia del contenido para preservar el original
            char contenido_copia[BYTES_POR_FILA];
            memset(contenido_copia, 0, sizeof(contenido_copia));
            strncpy(contenido_copia, contenido, sizeof(contenido_copia) - 1);

            // Parsear timestamp con verificación de NULL
            char *token_time = strtok(contenido_copia, ",");
            if (token_time && strlen(token_time) < sizeof(_savedData._DS18B20Data.time))
            {
                strcpy(_savedData._DS18B20Data.time, token_time);
            }
            else
            {
                Serial.println("Error: Token de tiempo inválido");
                strcpy(_savedData._DS18B20Data.time, "*");
            }

            // Parsear temperatura con verificación de NULL
            char *token_temp = strtok(NULL, ",");
            if (token_temp && strlen(token_temp) < sizeof(_savedData._DS18B20Data.temperature))
            {
                strcpy(_savedData._DS18B20Data.temperature, token_temp);
            }
            else
            {
                Serial.println("Error: Token de temperatura inválido");
                strcpy(_savedData._DS18B20Data.temperature, "*");
            }

            // Parsear energía con verificación de NULL
            char *token_energy = strtok(NULL, "^\n");
            if (token_energy && strlen(token_energy) < sizeof(_savedData.energy))
            {
                strcpy(_savedData.energy, token_energy);
            }
            else
            {
                Serial.println("Error: Token de energía inválido");
                strcpy(_savedData.energy, "*");
            }
            
            Serial.print("temp save => ");
            Serial.println(_savedData._DS18B20Data.temperature);
            Serial.print("timestamp save => ");
            Serial.println(_savedData._DS18B20Data.time);
            Serial.print("energy save => ");
            Serial.println(_savedData.energy);

            // _savedData._DS18B20Data.temperature = temperature_saved;
            // *temperature = atof(temperature_saved);
            // strcpy(_timestamp, timestamp_saved);
            // timestamp = timestamp_saved;

            /*Serial.print("temp => ");
            Serial.println(*temperature);
            Serial.print("timestamp => ");
            Serial.println(_timestamp);*/

            // char contenido_spiffs[30];                               // array del mismo tamaño que variable contenido. usada para almacenar una copia
            // strncpy(contenido_spiffs, contenido, strlen(contenido)); // copiar caracteres de contenido en contenido_spiffs, para que no se afecte la variable contenido al usar strtok desde la librería wifi_mqtt. esto ocurría a pesar de pasar el array de esta librería a la otra por valor (expresión del parámetro de la función de la otra librería), en realidad se pasaba por referencia

            //_Wifi_Mqtt.Envio_registro_sensores(contenido_spiffs, &next); // NOTA: a
            // memset(contenido_spiffs, '\0', sizeof contenido_spiffs); // borrar contenido para que no se concatene la información al copiar
        }
        else
        {
            if (SPIFFS.exists(fileData) == true && SPIFFS.exists(read_cursor_file_name) == true)
            {
                SPIFFS.remove(fileData);
                SPIFFS.remove(read_cursor_file_name);
                /*Serial.print("eliminación de archivos en SPIFFS de registro de mediciones y posicion de cursor: ");
                Serial.println(SPIFFS.remove(fileData));        // elimina el archivo de registro si fue leído en su totalidad. arroja true si fue eliminado exitosamente
                Serial.println(SPIFFS.remove(read_cursor_file_name)); // elimina el archivo donde se almacena la posicion del cursor durante la lectura las mediciones almacenadas en el spiffs
                Serial.println("Eliminados");*/
            }
        }
    }
    else
    {
        contador_ejecuciones = 0; // reinicio de la variable del contador de ejecuciones
        cont_registro = 0;        // contador de registros de mediciones (variable informativa, no necesaria)
        Serial.println("el archivo registro-mediciones no existe");

        if (SPIFFS.exists(read_cursor_file_name) == true)
        {
            SPIFFS.remove(read_cursor_file_name);
            /*Serial.print("eliminación de archivos en SPIFFS de posicion de cursor: ");
            Serial.println(SPIFFS.remove(read_cursor_file_name)); // elimina el archivo donde se almacena la posicion del cursor durante la lectura las mediciones almacenadas en el spiffs
            Serial.print("cursor eliminado");*/
        }
        // ESP.restart(); //reiniciar el ESP32 después de borrar los archivos de registro de sensores y del cursor
        // vTaskSuspend(NULL);
    }

    /*Serial.print("CONTADOR DE EJECUCIONES: ");
    Serial.println(contador_ejecuciones);*/
}

void funciones_spiffs::save_cursor_position(unsigned long posicion_cursor)
{
    // Verificar que la posición esté dentro de límites razonables
    if (posicion_cursor > FILAS_REGISTRO_TEMPERATURA * BYTES_POR_FILA)
    {
        Serial.println("Warning: Posición del cursor fuera de rango, limitando");
        posicion_cursor = 0;
    }

    File cursor_reg_sensores_save = SPIFFS.open(read_cursor_file_name, FILE_WRITE);
    if (!cursor_reg_sensores_save)
    {
        Serial.println("Error: No se pudo abrir archivo para guardar posición del cursor");
        return;
    }
    
    // Verificar que la escritura fue exitosa
    if (cursor_reg_sensores_save.print(posicion_cursor) <= 0)
    {
        Serial.println("Error: No se pudo escribir posición del cursor");
    }
    else
    {
        Serial.print("Posición del cursor guardada: ");
        Serial.println(posicion_cursor);
    }
    
    cursor_reg_sensores_save.close();
}

// LEER TODOS LOS SPIFFS
void funciones_spiffs::read_cursor_sensors(unsigned long *p_cursor_position)
{
    // Verificar que el puntero no sea NULL
    if (!p_cursor_position)
    {
        Serial.println("Error: Puntero de posición del cursor es NULL");
        return;
    }

    const int BUFFER_SIZE = 25; // Aumentar buffer para mayor seguridad
    char contenido[BUFFER_SIZE];
    memset(contenido, 0, BUFFER_SIZE); // Inicializar buffer con zeros
    int i = 0;
    bool existe_pos_fila_sensores = false;

    existe_pos_fila_sensores = SPIFFS.exists(read_cursor_file_name);

    if (existe_pos_fila_sensores == true)
    {
        File cursor_reg_sensores_read = SPIFFS.open(read_cursor_file_name, FILE_READ);
        if (!cursor_reg_sensores_read)
        {
            Serial.println("Error: No se pudo abrir archivo cursor");
            *p_cursor_position = 0;
            return;
        }

        // Leer con verificación de límites
        while (cursor_reg_sensores_read.available() && i < (BUFFER_SIZE - 1))
        {
            contenido[i] = cursor_reg_sensores_read.read();
            i++;
        }
        contenido[i] = '\0'; // Asegurar terminación nula
        cursor_reg_sensores_read.close();

        Serial.print("contenido cursor registro de sensores: ");
        Serial.println(contenido);

        // Verificar que el contenido no esté vacío
        if (strlen(contenido) > 0)
        {
            *p_cursor_position = atol(contenido);
            
            // Verificar que la posición sea válida
            if (*p_cursor_position > FILAS_REGISTRO_TEMPERATURA * BYTES_POR_FILA)
            {
                Serial.println("Warning: Posición del cursor fuera de rango, reiniciando a 0");
                *p_cursor_position = 0;
            }
        }
        else
        {
            Serial.println("Warning: Contenido del cursor vacío, reiniciando a 0");
            *p_cursor_position = 0;
        }
    }
    else
    {
        Serial.println("archivo cursor registro de sensores no existe. se va a iniciar con el caracter 0 como contenido");
        File cursor_reg_sensores_write = SPIFFS.open(read_cursor_file_name, FILE_WRITE);
        if (cursor_reg_sensores_write)
        {
            cursor_reg_sensores_write.print("0");
            cursor_reg_sensores_write.close();
        }
        else
        {
            Serial.println("Error: No se pudo crear archivo cursor");
        }
        *p_cursor_position = 0;
    }
}

void funciones_spiffs::Leer_Spiffs_gen(char *p_nombre_wifi, char *p_clave_wifi, char *p_nombre_clinica, char *p_nombre_area)
{
    // Serial.println("void leer_spiffs_gen");

    const int BUFFER_SIZE = 250; // Aumentar ligeramente el buffer para mayor seguridad
    char contenido[BUFFER_SIZE];
    memset(contenido, 0, BUFFER_SIZE); // Inicializar buffer con zeros
    unsigned int i = 0;

    File file_gen_read = SPIFFS.open("/general_config.txt");
    if (!file_gen_read)
    {
        Serial.println("Error: No se pudo abrir general_config.txt");
        return;
    }

    bool finish = false;
    int rows = 0;
    while (file_gen_read.available() && finish == false && i < (BUFFER_SIZE - 1))
    {
        contenido[i] = file_gen_read.read();

        if (contenido[i] == ':')
            rows++;

        if (rows == 4)
            if (contenido[i] == '^')
                finish = true;
        i++;
    }
    contenido[i] = '\0'; // Asegurar terminación nula
    file_gen_read.close();

    // Verificar que se leyeron los 4 campos esperados
    if (rows < 4)
    {
        Serial.println("Error: Formato de archivo incorrecto");
        return;
    }

    // Serial.println(contenido);

    char *token1 = strtok(contenido, ":");
    char *wifi_name = strtok(NULL, "^");
    char *token2 = strtok(NULL, ":");
    char *wifi_pass = strtok(NULL, "^");
    char *token3 = strtok(NULL, ":");
    char *clinic_name = strtok(NULL, "^");
    char *token4 = strtok(NULL, ":");
    char *area_name = strtok(NULL, "^");

    // Verificar que todos los tokens son válidos antes de copiar
    if (wifi_name && strlen(wifi_name) < 50) // Asumir límite de 50 caracteres
        strcpy(p_nombre_wifi, wifi_name);
    else
        Serial.println("Error: Nombre WiFi inválido");

    if (wifi_pass && strlen(wifi_pass) < 50)
        strcpy(p_clave_wifi, wifi_pass);
    else
        Serial.println("Error: Clave WiFi inválida");

    if (clinic_name && strlen(clinic_name) < 50)
        strcpy(p_nombre_clinica, clinic_name);
    else
        Serial.println("Error: Nombre clínica inválido");

    if (area_name && strlen(area_name) < 50)
        strcpy(p_nombre_area, area_name);
    else
        Serial.println("Error: Nombre área inválido");

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
    const int MAX_FILENAME = 35;
    char name_file[MAX_FILENAME];
    memset(name_file, 0, MAX_FILENAME);

    // Serial.println("Reading calibration points");
    // Serial.println(type);

    // Verificar longitud del tipo
    if (!type || strlen(type) > 20)
    {
        Serial.println("Error: Tipo de sensor inválido");
        return;
    }

    // Usar snprintf para mayor seguridad
    int result = snprintf(name_file, MAX_FILENAME, "/cal_%s.txt", type);
    if (result >= MAX_FILENAME || result < 0)
    {
        Serial.println("Error: Nombre de archivo demasiado largo");
        return;
    }

    // Serial.println(name_file);

    File file_cal_read = SPIFFS.open(name_file);
    if (!file_cal_read)
    {
        Serial.println("Error: No se pudo abrir archivo de calibración");
        return;
    }

    const int BUFFER_SIZE = 60; // Aumentar buffer para mayor seguridad
    char contenido[BUFFER_SIZE];
    memset(contenido, 0, BUFFER_SIZE);
    unsigned int i = 0;

    bool finish = false;
    while (file_cal_read.available() && finish == false && i < (BUFFER_SIZE - 1))
    {
        contenido[i] = file_cal_read.read();
        if (contenido[i] == '^')
            finish = true;
        else
            i++;
    }
    contenido[i] = '\0'; // Asegurar terminación nula
    file_cal_read.close();

    // Verificar que se leyó contenido válido
    if (strlen(contenido) == 0)
    {
        Serial.println("Error: Archivo de calibración vacío");
        return;
    }

    // Serial.println(contenido);

    // Usar strtok con verificación de punteros nulos
    char *token = strtok(contenido, ",");
    if (token)
        values[0] = atof(token);
    else
    {
        Serial.println("Error: Token 0 inválido");
        return;
    }

    for (int i = 1; i < 5; i++)
    {
        token = strtok(NULL, ",");
        if (token)
            values[i] = atof(token);
        else
        {
            Serial.println("Error: Token inválido en posición " + String(i));
            return;
        }
    }

    token = strtok(NULL, "^");
    if (token)
        values[5] = atof(token);
    else
    {
        Serial.println("Error: Token final inválido");
        return;
    }
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

    const int BUFFER_SIZE = 110; // Aumentar buffer para mayor seguridad
    char contenido[BUFFER_SIZE];
    memset(contenido, 0, BUFFER_SIZE);
    unsigned int i = 0;

    File file_cal_read = SPIFFS.open("/cal_temperatura.txt");
    if (!file_cal_read)
    {
        Serial.println("Error: No se pudo abrir archivo de calibración de temperatura");
        return;
    }

    bool finish = false;
    while (file_cal_read.available() && finish == false && i < (BUFFER_SIZE - 1))
    {
        contenido[i] = file_cal_read.read();
        if (contenido[i] == '^')
            finish = true;
        else
            i++;
    }
    contenido[i] = '\0'; // Asegurar terminación nula

    file_cal_read.close();

    // Verificar que se leyó contenido válido
    if (strlen(contenido) == 0)
    {
        Serial.println("Error: Archivo de calibración vacío");
        return;
    }

    // Serial.println(contenido);

    // Usar strtok con verificación de punteros nulos y longitud máxima
    const int MAX_VALUE_LENGTH = 20; // Longitud máxima esperada para cada valor

    char *token = strtok(contenido, ",");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(p_value_1, token);
    else
    {
        Serial.println("Error: Valor 1 inválido");
        strcpy(p_value_1, "0");
    }

    token = strtok(NULL, ",");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(p_value_2, token);
    else
    {
        Serial.println("Error: Valor 2 inválido");
        strcpy(p_value_2, "0");
    }

    token = strtok(NULL, ",");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(p_value_3, token);
    else
    {
        Serial.println("Error: Valor 3 inválido");
        strcpy(p_value_3, "0");
    }

    token = strtok(NULL, ",");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(m_value_1, token);
    else
    {
        Serial.println("Error: Valor M1 inválido");
        strcpy(m_value_1, "0");
    }

    token = strtok(NULL, ",");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(m_value_2, token);
    else
    {
        Serial.println("Error: Valor M2 inválido");
        strcpy(m_value_2, "0");
    }

    token = strtok(NULL, "^");
    if (token && strlen(token) < MAX_VALUE_LENGTH)
        strcpy(m_value_3, token);
    else
    {
        Serial.println("Error: Valor M3 inválido");
        strcpy(m_value_3, "0");
    }
}

void funciones_spiffs::Leer_Spiffs_name_card(char *name_card)
{
    // Serial.println("Leer nombre de la tarjeta");

    const int BUFFER_SIZE = 70; // Aumentar buffer para mayor seguridad
    char contenido[BUFFER_SIZE];
    memset(contenido, 0, BUFFER_SIZE);
    unsigned int i = 0;

    File file_name_read = SPIFFS.open("/pcb_name.txt");
    if (!file_name_read)
    {
        Serial.println("Error: No se pudo abrir archivo de nombre de tarjeta");
        return;
    }

    bool finish = false;
    while (file_name_read.available() && finish == false && i < (BUFFER_SIZE - 1))
    {
        contenido[i] = file_name_read.read();
        if (contenido[i] == '^')
            finish = true;
        else
            i++;
    }
    contenido[i] = '\0'; // Asegurar terminación nula
    file_name_read.close();

    // Verificar que se leyó contenido válido
    if (strlen(contenido) == 0)
    {
        Serial.println("Error: Archivo de nombre vacío");
        strcpy(name_card, "DEFAULT"); // Valor por defecto
        return;
    }

    // Serial.println(contenido);

    char *token = strtok(contenido, "^");
    if (token && strlen(token) < 50) // Verificar longitud máxima del nombre
    {
        strcpy(name_card, token);
    }
    else
    {
        Serial.println("Error: Nombre de tarjeta inválido");
        strcpy(name_card, "DEFAULT");
    }
}
