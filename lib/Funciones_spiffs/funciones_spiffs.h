/*Librería con funciones del archivo "Interrupción de botones" ubicado en la ruta: C:\Users\Usuario\Desktop\Pruebas Andrés\Firmware\Integración con edición de tópicos SPIFFS\Inte...SPIFFS 1
Función almacenar_spiffs agregada
Función leer_spiffs agregada
*/
// 42.3  txt

#ifndef FUNCIONES_SPIFFS
#define FUNCIONES_SPIFFS

// #define BYTES_PER_ROW 36 // número de caracteres o bytes que hay en cada fila del registro de datos, depende de cuantos datos queramos guardar, sacando el caracter nulo
// #define BYTES_STORE 1570864 // número de bytes destinados para guardar información de sensores, dejando 6000 para información de configuración

#include <Arduino.h>
#include <iostream>
#include <vector>
#include "SPIFFS.h"
#include "config.h"
#include "DS18B20HS.h"
#include <ArduinoJson.h>
#define TAMANO_CONFIG_GEN 50

#define BYTES_POR_FILA 30 // número de caracteres o bytes que hay en cada fila del registro de datos
// #define FILAS_REGISTRO_TEMPERATURA 41200 // número de filas del archivo de registro en SPIFFS para datos de temperatura para 1 mes, con guardado de mediciones cada 1 minuto
#define FILAS_REGISTRO_TEMPERATURA 11027 // número de filas del archivo de registro en SPIFFS para datos de temperatura para 1 mes, con guardado de mediciones cada 1 minuto

extern bool firmware;
extern bool flag_callback_broker;

extern String location;
extern const char *fileData;
// extern DS18B20Saved savedData;

class funciones_spiffs
{
private:
  // char data_file_name[17] = "/device-data.txt";
  char read_cursor_file_name[17] = "/read-cursor.txt";

  void update_device(JsonDocument json);
  void save_frequency(JsonDocument json);
  void save_device_name(JsonDocument json);
  void save_alarm_limits(JsonDocument json);
  void save_general_settings(JsonDocument json);
  void save_calibration_points(JsonDocument json);

  std::vector<String> getData();

public:
  void delete_first_data();
  bool read_data(DS18B20Saved &);
  void process_file(JsonDocument json);
  // void save_data(String, const DS18B20Data &, int &);
  void save_cursor_position(unsigned long);
  void save_sensor_data(String, const DS18B20Data &, char);
  void read_sensor_register(bool , DS18B20Saved &);
  bool saveAlertWithTimestamp(const char *timestamp, const char *type);

  void load_all_alarm_limits();
  void read_cursor_sensors(unsigned long *);
  void Leer_Spiffs_sen(float *);
  void Leer_Spiffs_name_card(char *);
  void load_alarm_limits(const char *, int &, int &);
  void readCalibrationPoints(const char *, float[6]);
  void Leer_Spiffs_gen(char *, char *, char *, char *);
  void Leer_Spiffs_cal(char *, char *, char *, char *, char *, char *, int);
};

#endif