
#include "BLS_AP.h"

hw_timer_t *timer_zero_AP = NULL;

// static NimBLEServer *pServer;

BLEServer *pServer = NULL;
BLECharacteristic *pSensorCharacteristic = NULL;
BLECharacteristic *pLedCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// procesar_tramas _pt;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        led_flashing_lock_time = pdMS_TO_TICKS(500); // parpadeo cuando se conectó un cliente al punto de acceso
        timerStop(timer_zero_AP);                    // detener el timer al conectarse
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        led_flashing_lock_time = pdMS_TO_TICKS(100); // Cliente se desconectó
        timerStart(timer_zero_AP);                   // iniciar el timer al desconectarse
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    /*void onWrite(BLECharacteristic* pLedCharacteristic) {
        String value = pLedCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("Characteristic event, written: ");
            Serial.println(static_cast<int>(value[0])); // Print the integer value

            int receivedValue = static_cast<int>(value[0]);
            if (receivedValue == 1) {
                digitalWrite(ledPin, HIGH);
            } else {
                digitalWrite(ledPin, LOW);
            }
        }
    }*/

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

                // _pt.tramaBluetooth(doc);

                Serial.println("tramaBluetooth");
                configuration_updated = true;
                // flag_callback_broker = true;
            }

            // Set the flag to indicate configuration update
            //configuration_updated = true;
        }
    }
};

void BLS_AP::Access_Point(bool *p_estado_servidor, void Idle_Reboot_AP())
{
    Serial.println("Inicio servidor bluetooth");

    timer_zero_AP = timerBegin(0, 80, true);
    timerAttachInterrupt(timer_zero_AP, Idle_Reboot_AP, true);
    timerAlarmWrite(timer_zero_AP, 1000000 * 120, true); // reinicio del esp32 después de 2 minutos de abierto el punto de acceso
    timerAlarmEnable(timer_zero_AP);                     // inicio del timer que limita el tiempo en que permanecerá activo el punto de acceso para configurar el ESP32

    // Create the BLE Device
    BLEDevice::init("HelpSmart");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    /*pSensorCharacteristic = pService->createCharacteristic(
        SENSOR_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);*/

    // Create the ON button Characteristic
    /*pLedCharacteristic = pService->createCharacteristic(
                                            LED_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );*/

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
    // pLedCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    // pConfigCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    // Create a BLE Descriptor
    // pSensorCharacteristic->addDescriptor(new BLE2902());
    // pLedCharacteristic->addDescriptor(new BLE2902());
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
