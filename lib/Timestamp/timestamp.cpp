#include "timestamp.h"

//ESP32Time rtc;

void Timestamp::synchronizeTimestamp()
{
  HTTPClient http;

  http.begin(API_TIME); // Reemplaza "URL_DEL_API" con la URL del API que proporciona la fecha y hora
  int httpResponseCode = http.GET();
  int retries = 0;
  while (retries < 3)
  {
    if (httpResponseCode > 0)
    {
      String datetime = http.getString();

      int year, month, day, hour, minute, second;
      sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

      rtc.setTime(second, minute, hour, day, month, year); // 17th Jan 2021 15:24:30

      Serial.println("Fecha y hora sincronizadas");
      break;
    }
    else
      retries++, Serial.println("Error al sincronizar la fecha y hora");
  }
  http.end();
}

String Timestamp::getTime()
{
  return rtc.getTime("%Y-%m-%d %H:%M:%S");
}