
#include "BLS_AP.h"

hw_timer_t *timer_zero_AP = NULL;

static NimBLEServer *pServer;

// procesar_tramas _pt;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer)
    {
        NimBLEDevice::startAdvertising();

        // use_led = true;
        led_flashing_lock_time = pdMS_TO_TICKS(500); // parpadeo cuando se conectó un cliente al punto de acceso

        // parar contador de tiempo
        timerRestart(timer_zero_AP); // reinicia el temporizador si el cliente se desconecta
        timerStop(timer_zero_AP);    // detener el timer al conectarse
    };
    /** Alternative onConnect() method to extract details of the connection.
     *  See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
     */
    void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
    {
        Serial.print("Client address: ");
        Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());

        pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    };
    void onDisconnect(NimBLEServer *pServer)
    {
        Serial.println("Client disconnected server - start advertising");
        NimBLEDevice::startAdvertising();

        // use_led = true;
        led_flashing_lock_time = pdMS_TO_TICKS(100); // Cliente se desconectó

        // iniciar contador de tiempo
        timerStart(timer_zero_AP); // iniciar el timer al desconectarse
    };
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc *desc)
    {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest()
    {
        Serial.println("Server Passkey Request");
        /** This should return a random 6 digit number for security
         *  or make your own static passkey as done here.
         */
        return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key)
    {
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
        /** Return false if passkeys don't match. */
        return true;
    };

    void onAuthenticationComplete(ble_gap_conn_desc *desc)
    {
        /** Check that encryption was successful, if not we disconnect the client */
        if (!desc->sec_state.encrypted)
        {
            NimBLEDevice::getServer()->disconnect(desc->conn_handle);
            Serial.println("Encrypt connection failed - disconnecting client");
            return;
        }

        // use_led = true;

        Serial.println("Starting BLE work!");
    };
};

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onWrite(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());

        // transformar valor de la caracteristica en json
        String json = pCharacteristic->getValue().c_str();
        //StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc_config, json);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
        }
        // obtener valor de la caracteristica
        //String comand = doc_config["general_config"].as<String>();
        //String comand = doc_config["comand"].as<String>();

        serializeJson(doc_config, Serial);

        // quitar espacios en blanco
        //comand.trim();
        //comand.toLowerCase();

        //if (comand == "test")
        if (doc_config["test"])
            Serial.println("test");
        //else if (comand == "cg")
        else if (doc_config["general_config"])
        {
            Serial.println("cg");

            // _pt.tramaBluetooth(doc);

            Serial.println("tramaBluetooth");
            configuration_updated = true;
            // flag_callback_broker = true;
        }
        //else if (comand == "mac")
        else if (doc_config["mac"])
        {
            Serial.println("mac");
            // get mac address wifi
            String mac = WiFi.macAddress();
            // enviar mac address wifi al cliente

            Serial.println(mac);

            if (pServer->getConnectedCount())
            {
                NimBLEService *pSvc = pServer->getServiceByUUID("BAAD");
                if (pSvc)
                {
                    NimBLECharacteristic *pChr = pSvc->getCharacteristic("F00D");
                    if (pChr)
                    {
                        Serial.println("pChr->setValue");
                        pChr->setValue(mac);
                        pChr->notify(true);
                    }
                }
            }
        }
        else
        {
            Serial.println("comando no encontrado");
        }

        // close connection
        /*pServer->disconnect(0);
        NimBLEDevice::deinit(true);*/

        timerDetachInterrupt(timer_zero_AP); // Detener las interrupcipones para el timer 0
        timerAlarmDisable(timer_zero_AP);    // Deshalibitar el timer 0
        timerEnd(timer_zero_AP);             // Borrar el timer 0
        NimBLEDevice::deinit(true);

        Serial.println("Configurado");
    };

    void onNotify(NimBLECharacteristic *pCharacteristic)
    {
        Serial.println("Sending notification to clients");
    };

    void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
    {
        String str = ("Notification/Indication status code: ");
        str += status;
        str += ", return code: ";
        str += code;
        str += ", ";
        str += NimBLEUtils::returnCodeToString(code);
        Serial.println(str);
    };

    void onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue)
    {
        String str = "Client ID: ";
        str += desc->conn_handle;
        str += " Address: ";
        str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
        if (subValue == 0)
        {
            str += " Unsubscribed to ";
        }
        else if (subValue == 1)
        {
            str += " Subscribed to notfications for ";
        }
        else if (subValue == 2)
        {
            str += " Subscribed to indications for ";
        }
        else if (subValue == 3)
        {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic->getUUID()).c_str();

        Serial.println(str);
    };
};

/** Handler class for descriptor actions */
class DescriptorCallbacks : public NimBLEDescriptorCallbacks
{
    void onWrite(NimBLEDescriptor *pDescriptor)
    {
        std::string dscVal = pDescriptor->getValue();
        Serial.print("Descriptor witten value:");
        Serial.println(dscVal.c_str());
    };

    void onRead(NimBLEDescriptor *pDescriptor)
    {
        Serial.print(pDescriptor->getUUID().toString().c_str());
        Serial.println(" Descriptor read");
    };
};

/** Define callback instances globally to use for multiple Charateristics \ Descriptors */
static DescriptorCallbacks dscCallbacks;
static CharacteristicCallbacks chrCallbacks;

void BLS_AP::Access_Point(bool *p_estado_servidor, void Idle_Reboot_AP())
{

    Serial.println("Inicio servidor bluetooth");

    timer_zero_AP = timerBegin(0, 80, true);
    timerAttachInterrupt(timer_zero_AP, Idle_Reboot_AP, true);
    timerAlarmWrite(timer_zero_AP, 1000000 * 120, true); // reinicio del esp32 después de 2 minutos de abierto el punto de acceso
    timerAlarmEnable(timer_zero_AP);                     // inicio del timer que limita el tiempo en que permanecerá activo el punto de acceso para configurar el ESP32

    /** sets device name */
    NimBLEDevice::init("Help Smart");

    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pDeadService = pServer->createService("DEAD");
    NimBLECharacteristic *pBeefCharacteristic = pDeadService->createCharacteristic(
        "BEEF",
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            /** Require a secure connection for read and write access */
            NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
            NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
    );

    pBeefCharacteristic->setValue("Burger");
    pBeefCharacteristic->setCallbacks(&chrCallbacks);

    /** 2904 descriptors are a special case, when createDescriptor is called with
     *  0x2904 a NimBLE2904 class is created with the correct properties and sizes.
     *  However we must cast the returned reference to the correct type as the method
     *  only returns a pointer to the base NimBLEDescriptor class.
     */
    NimBLE2904 *pBeef2904 = (NimBLE2904 *)pBeefCharacteristic->createDescriptor("2904");
    pBeef2904->setFormat(NimBLE2904::FORMAT_UTF8);
    pBeef2904->setCallbacks(&dscCallbacks);

    NimBLEService *pBaadService = pServer->createService("BAAD");
    NimBLECharacteristic *pFoodCharacteristic = pBaadService->createCharacteristic(
        "F00D",
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY);

    pFoodCharacteristic->setValue("init");
    pFoodCharacteristic->setCallbacks(&chrCallbacks);

    /** Note a 0x2902 descriptor MUST NOT be created as NimBLE will create one automatically
     *  if notification or indication properties are assigned to a characteristic.
     */

    /** Custom descriptor: Arguments are UUID, Properties, max length in bytes of the value */
    NimBLEDescriptor *pC01Ddsc = pFoodCharacteristic->createDescriptor(
        "C01D",
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_ENC, // only allow writing if paired / encrypted
        20);
    pC01Ddsc->setValue("Send it back!");
    pC01Ddsc->setCallbacks(&dscCallbacks);

    /** Start the services when finished creating all Characteristics and Descriptors */
    pDeadService->start();
    pBaadService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pDeadService->getUUID());
    pAdvertising->addServiceUUID(pBaadService->getUUID());
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}

void BLS_AP::close_settings()
{
    timerDetachInterrupt(timer_zero_AP); // Detener las interrupcipones para el timer 0
    timerAlarmDisable(timer_zero_AP); // Deshalibitar el timer 0
    timerEnd(timer_zero_AP);          // Borrar el timer 0
    NimBLEDevice::deinit(true);

    Serial.println("Modo configuracion cerrado");
}