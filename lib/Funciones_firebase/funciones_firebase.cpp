#include "funciones_firebase.h"

HTTPClient http;
funciones_spiffs _spiffs;

void fire::creation_json_alert(char *p_token_de_app, char *value, char *p_mensaje_notificacion, char *p_trama_json)
{
  //char token_de_app[301] = "*";
  //char mensaje_notificacion[150] = "*";
  //strcpy(token_de_app, p_token_de_app);
  //strcpy(mensaje_notificacion, p_mensaje_notificacion);

  JsonDocument doc;
  JsonObject message = doc["message"].to<JsonObject>();

  // Crear el campo "notification" con sus subcampos
  // JsonObject notification = message.createNestedObject("notification");
  // JsonObject mnotify = doc["message"].to<JsonObject>();
  // JsonObject notify = doc.createNestedObject("notification");
  JsonObject notify = message["notification"].to<JsonObject>();
  notify["title"] = "Notificación de alerta";
  notify["body"] = p_mensaje_notificacion;
  notify["image"] = "https://i.ibb.co/pXp35Bj/Logo-HM-litte.png";
  notify["sound"] = "true";

  doc["priority"] = "high";

  // JsonObject data = doc.createNestedObject("data");
  JsonObject data = message["data"].to<JsonObject>();
  data["click_action"] = "FLUTTER_NOTIFICATION_CLICK";
  data["id"] = 1;
  data["status"] = "done";
  data["alerta"] = p_mensaje_notificacion;
  data["value"] = value;

  doc["to"] = p_token_de_app;

  char buffer[1024];
  serializeJson(doc, buffer);
  strcpy(p_trama_json, buffer);
}

void fire::send_notify(bool helpsmart, char *value, char *p_mensaje_notificacion)
{
  char trama_json[1024] = "*";
  char token_de_app[50] = "*"; //, token_de_firebase[301] = "";

  //_spiffs.Leer_Spiffs_gen(token_de_app, token_de_firebase);
  if (helpsmart == false)
    _spiffs.Leer_Spiffs_gen(token_de_app);
  else
    strcpy(token_de_app, "/topics/Helpmedica");

  Serial.println(token_de_app);

  creation_json_alert(token_de_app, value, p_mensaje_notificacion, trama_json);

  /*int httpResponseCode = http.POST(trama_json);

  for (int i = 1; i <= 5; i++)
  {
    if (httpResponseCode != 200)
    {
      http.begin("https://fcm.googleapis.com/fcm/send");
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Authorization", token_de_firebase);
      httpResponseCode = http.POST(trama_json);
      Serial.print("HTTP Response code Firebase: ");
      Serial.println(httpResponseCode);
      http.end();

      delay(500);
    }
    else if (httpResponseCode == 200)
      i = 6;
  }*/
}

// /topics/Helpmedica