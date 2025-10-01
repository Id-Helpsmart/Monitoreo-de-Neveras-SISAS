#include "Sensores.h"

funciones_spiffs spiffs;

static float lastTemperature = 0.0;

void Sensores::Setup_Sen()
{
    pinMode(PIN_4_INPUT_ENERGY, INPUT_PULLDOWN);
    pinMode(PIN_36_BATTERY_LEVEL, INPUT);
    //analogReadResolution(10); // resolución del conversor ADC para la lectura de la batería

    spiffs.Leer_Spiffs_name_card(name_card);

    spiffs.readCalibrationPoints(keyValueToString(keyValues::TEMPERATURE), values);
    /*for (int i = 0; i < 6; i++) {
        Serial.print("Value ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(values[i]);
    }*/
    /*_pt_value_1 = atof(pt_value[0]); 0
    _pt_value_2 = atof(pt_value[1]); 1
    _pt_value_3 = atof(pt_value[2]); 2
    _mt_value_1 = atof(mt_value[0]); 3
    _mt_value_2 = atof(mt_value[1]); 4
    _mt_value_3 = atof(mt_value[2]);*/
    // 5

    mt1 = (values[1] - values[0]) / (values[4] - values[3]);
    mt2 = (values[2] - values[1]) / (values[5] - values[4]);
    /*Serial.println("Calibración");
    Serial.println(mt1);
    Serial.println(mt2);*/
}

void Sensores::readData(DS18B20Data &envData)
{
    DS18B20HS _DS18B20;
    // DS18B20Data envData;

    _DS18B20.getData(envData);

    float temperature = atof(envData.temperature);

    if (lastTemperature == 0.0)
        lastTemperature = temperature; // initialize on first call

    while (fabs(temperature - lastTemperature) > threshold)
    {
        _DS18B20.getData(envData);
        temperature = atof(envData.temperature);
        delay(100);
        // if (temperature > lastTemperature)
        //     temperature = lastTemperature + threshold;
        // else
        //     temperature = lastTemperature - threshold;
    }
    
    lastTemperature = temperature;

    if (temperature < values[4])
    temperature = mt1 * temperature - (mt1 * values[3]) + (values[0]);
    if (temperature >= values[4])
    temperature = mt2 * temperature - (mt2 * values[4]) + (values[1]);
    
    dtostrf(temperature, 5, 2, envData.temperature);
}

void Sensores::Update_Notify()
{
    strcpy(notification_message, "Actualización de firmware completada. ");
    strcat(notification_message, name_card);
    // _fire.send_notify(true, on, notification_message);
}

bool Sensores::Input_Energy(volatile bool *p_emergency)
{
    state_energy = digitalRead(PIN_4_INPUT_ENERGY);

    if (previous_state_energy == false && state_energy == true)
        Serial.println("Energía Restablecida "), *p_emergency = true;
    else if (previous_state_energy == true && state_energy == false)
        Serial.println("Energía Ausente "), *p_emergency = true;

    previous_state_energy = state_energy;

    // Serial.println(state_energy ? "Energía ON" : "Energía OFF");
    return state_energy;
}

float Sensores::Battery_Level(volatile bool *p_emergency)
{
    int sensorValue = analogRead(PIN_36_BATTERY_LEVEL);

    for (int i = 0; i < 10; i++)
    {
        battery_voltage = battery_voltage + (sensorValue * (battery_max / 1023.0));
    }

    battery_level = battery_voltage / 10;

    if (battery_level <= battery_min)
        battery_level = battery_min;

    voltage_percent = ((battery_level - battery_min) / (battery_max - battery_min)) * 100;
    voltage_percent = (int(voltage_percent * 10)) / 10.0;

    if (battery_level <= voltage_min)
    {
        /*if (b_alarm == true)
        {
            dtostrf(voltage_percent, 3, 0, voltaje_char);
            strcpy(notification_message, "Batería del sensor baja ");
            strcat(notification_message, voltaje_char);
            strcat(notification_message, " % ");
            strcat(notification_message, name_card);
            _fire.envio_notificacion(&voltage_percent, notification_message);

            b_alarm = 0;
            *p_emergency = true;
        }*/
    }
    else if (battery_level > voltage_min && battery_level < battery_max)
        b_alarm = 1;

    battery_voltage = 0;

    return voltage_percent;
}