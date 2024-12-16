#include "Manager.h"
#include <Arduino.h>
#include <Wire.h>

String _NetworkName;
String Mac;
byte mac[6];

byte detector = 0xf;

String Manager::id_lora()
{
  WiFi.macAddress(mac);
  Mac = (String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX));
  return Mac;
}

String Manager::id()
{

  Mac = "";
  WiFi.macAddress(mac);

  for (int i = 0; i < 6; i++)
  {
    if (mac[i] <= detector)
      Mac += "0" + String(mac[i], HEX);
    else
      Mac += String(mac[i], HEX);

    if (i < 5)
      Mac += ":";
  }

  // quitar espacios en blanco
  Mac.trim();
  Mac.toLowerCase();

  return Mac;
}

String Manager::NetworkName()
{
  _NetworkName = "HM:T:" + id_lora().substring(7, 12);
  return _NetworkName;
}