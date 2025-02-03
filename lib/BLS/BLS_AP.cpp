
#include "BLS_AP.h"
#include "nvs_flash.h"

BLEServer *pServer = NULL;
hw_timer_t *timer_zero_AP = NULL;
BLECharacteristic *pLedCharacteristic = NULL;
BLECharacteristic *pSensorCharacteristic = NULL;

uint32_t value = 0;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// procesar_tramas _pt;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        // Serial.println("Cliente conectado");
        deviceConnected = true;
        use_led = true;
        led_flashing_lock_time = pdMS_TO_TICKS(500); // parpadeo cuando se conectó un cliente al punto de acceso
        timerRestart(timer_zero_AP);                 // reiniciar el timer al conectarse
        timerStop(timer_zero_AP);                    // detener el timer al conectarse
    };

    void onDisconnect(BLEServer *pServer)
    {
        // Serial.println("Cliente desconectado");
        deviceConnected = false;
        led_flashing_lock_time = pdMS_TO_TICKS(100); // Cliente se desconectó
        timerStart(timer_zero_AP);                   // iniciar el timer al desconectarse
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pConfigCharacteristic)
    {
        std::string value = pConfigCharacteristic->getValue();
        if (value.length() > 0)
        {
            Serial.print("Configuration JSON received: ");
            Serial.println(value.c_str());

            // Parse the JSON configuration
            DeserializationError error = deserializeJson(doc_config, value);
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            if (doc_config["general_config"])
            {
                Serial.println("cg");

                configuration_updated = true;
                // flag_callback_broker = true;
            }

            // Set the flag to indicate configuration update
            // configuration_updated = true;
        }
    }
};

void BLS_AP::deInitBLE()
{
    timerRestart(timer_zero_AP);
    timerStop(timer_zero_AP);
    Serial.println("Intentando detener el servidor BLE...");

    if (pServer)
    {

        if (deviceConnected)
        {
            Serial.println("Aún hay un dispositivo conectado. Desconectando...");
            pServer->disconnect(0); // Forzar desconexión del cliente
            delay(500);             // Espera un poco para asegurar que la desconexión se procesa
        }

        Serial.println("Deteniendo servidor Bluetooth...");
        BLEDevice::stopAdvertising(); // Detiene la publicidad BLE
        delay(100);
        BLEDevice::deinit(); // Apaga el stack BLE
        pServer = nullptr;
        Serial.println("Servidor BLE detenido con éxito.");
        ESP.restart();
    }
}

void IRAM_ATTR flagClose()
{
    Serial.println("Timer finalizado");
    // ESP.restart();
    server_close = true;
}

// void BLS_AP::Access_Point(bool *p_estado_servidor, void Idle_Reboot_AP())
void BLS_AP::Access_Point(bool *p_estado_servidor)
{

    if (timer_zero_AP == NULL)
    {
        timer_zero_AP = timerBegin(0, 80, true);
        timerAttachInterrupt(timer_zero_AP, flagClose, true);
        timerAlarmWrite(timer_zero_AP, 1000000 * 120, true); // reinicio del esp32 después de 2 minutos de abierto el punto de acceso
        timerAlarmEnable(timer_zero_AP); // inicio del timer que limita el tiempo en que permanecerá activo el punto de acceso para configurar el ESP32
    }
    else
        timerStart(timer_zero_AP), Serial.println("Timer iniciado");

    Serial.println("Inicio servidor bluetooth");

    *p_estado_servidor = true;

    // Create the BLE Device
    BLEDevice::init("HelpSmart");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create the configuration JSON Characteristic
    BLECharacteristic *pConfigCharacteristic = pService->createCharacteristic(
        CONFIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    // Register the callback for the configuration characteristic
    pConfigCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    // Register the callback for the ON button characteristic
    pConfigCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}
