#include "DS18B20HS.h"

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

DS18B20HS::DS18B20HS()
{
  sensor.begin();
  sensor.setResolution(12);
}

void DS18B20HS::getData(DS18B20Data &envData)
{
  // DS18B20Data envData;
  sensor.requestTemperatures();

  //  wait until sensor is ready
  while (!sensor.isConversionComplete())
  {
    delay(1);
  }

  float temperature = sensor.getTempC();
  //return envData;

  if (temperature > 125 || temperature <= -55) // si la temperatura es mayor a 100 o menor o igual a 0, se asigna un valor de NAN
    {
        message_trouble = "Temperatura fuera de rango " + String(temperature), Serial.println(message_trouble);
        trouble = true;
        return;
    }
    
    dtostrf(temperature, 6, 2, envData.temperature);
}

// Función que convierte un keyValueToString
const char *keyValueToString(keyValues::Type keyValue)
{
  switch (keyValue)
  {

  case keyValues::TEMPERATURE:
    return "TEMPERATURA";
  default:
    return "UNKNOWN";
  }
}

// Definir la variable alarmLimits
std::map<keyValues::Type, AlarmLimits> alarmLimits = {
    {keyValues::TEMPERATURE, {0, 100}},
};

DS18B20HS::~DS18B20HS()
{
}