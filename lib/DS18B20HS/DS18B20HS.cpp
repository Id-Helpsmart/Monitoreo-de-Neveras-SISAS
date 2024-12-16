#include "DS18B20HS.h"

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

DS18B20HS::DS18B20HS()
{
  sensor.begin();
  sensor.setResolution(12);
}

DS18B20Data DS18B20HS::getData()
{
  DS18B20Data envData;
  sensor.requestTemperatures();

  //  wait until sensor is ready
  while (!sensor.isConversionComplete())
  {
    delay(1);
  }

  envData.temperature = sensor.getTempC();
  return envData;

  //return sensor.getTempC();
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