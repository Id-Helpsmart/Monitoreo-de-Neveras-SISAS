#ifndef FUNCIONES_FIREBASE
#define FUNCIONES_FIREBASE
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "funciones_spiffs.h"

class fire
{
private:
    char token_de_firebase[200] = "key=AAAASMVslhg:APA91bEJkZWTR7s4ukfRDIWGVHv_xSUihu-1WDCPMFQUv7BMAWWP5KWosn-rqn93zFECD-BRDTkqZj-UTm2K2ovfOXM_9QS9lgK6g803s3eOM5NrPbUN1unvPu2fHCcWNlJiiJPtLzfm";

    void creation_json_alert(char *p_token_de_app, char *value, char *p_mensaje_notificacion, char *p_trama_json);

public:
    void send_notify(bool helpsmart, char *value, char *p_mensaje_notificacion);
};

#endif