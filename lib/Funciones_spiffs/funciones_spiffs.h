/*Librería con funciones del archivo "Interrupción de botones" ubicado en la ruta: C:\Users\Usuario\Desktop\Pruebas Andrés\Firmware\Integración con edición de tópicos SPIFFS\Inte...SPIFFS 1
Función almacenar_spiffs agregada
Función leer_spiffs agregada
*/
// 42.3  txt

#ifndef FUNCIONES_SPIFFS
#define FUNCIONES_SPIFFS

#define BYTES_PER_ROW 36    // número de caracteres o bytes que hay en cada fila del registro de datos, depende de cuantos datos queramos guardar, sacando el caracter nulo
// #define BYTES_STORE 1570864 // número de bytes destinados para guardar información de sensores, dejando 6000 para información de configuración

#include <Arduino.h>
#include <iostream>
#include <vector>
#include "SPIFFS.h"
#include "config.h"
#include "DS18B20HS.h"
#include <ArduinoJson.h>
#define TAMANO_CONFIG_GEN 50

extern bool firmware;
extern bool flag_callback_broker;

extern String location;
extern const char *fileData;
extern DS18B20Saved savedData;

class funciones_spiffs
{
private:
  void update_device(JsonDocument json);
  void save_frequency(JsonDocument json);
  void save_device_name(JsonDocument json);
  void save_alarm_limits(JsonDocument json);
  void save_general_settings(JsonDocument json);
  void save_calibration_points(JsonDocument json);
  
  std::vector<String> getData();

public:
  void delete_first_data();
  bool read_data(DS18B20Saved *);
  void process_file(JsonDocument json);
  void save_data(String, const DS18B20Data *, int);
  bool saveAlertWithTimestamp(const char *timestamp, const char *type);

  void load_all_alarm_limits();
  void readCalibrationPoints(const char *type, float [6]);
  void Leer_Spiffs_gen(char *p_token_de_app); // función sobrecargada. recibe un número de argumentos distinto
  void Leer_Spiffs_name_card(char *name_card);
  void Leer_Spiffs_sen(float *p_publish_freq_num);
  void Leer_Spiffs_gen(char *p_nombre_wifi, char *p_clave_wifi, char *p_nombre_clinica, char *p_nombre_area);
  void load_alarm_limits(const char *, int &, int &);
  void Leer_Spiffs_cal(char *p_value_1, char *p_value_2, char *p_value_3, char *m_value_1, char *m_value_2, char *m_value_3, int campo_del_sensor);

};

#endif