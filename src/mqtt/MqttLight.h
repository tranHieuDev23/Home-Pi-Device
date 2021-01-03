#ifndef MQTT_LIGHT_H
#define MQTT_LIGHT_H

#include "MqttDevice.h"
#include <ArduinoJson.h>

class MqttLight : public MqttDevice
{
private:
    const uint8_t lightPin;

public:
    MqttLight(const std::shared_ptr<PubSubClient> &mqttClient, uint8_t lightPin) : MqttDevice(mqttClient), lightPin(lightPin)
    {
        pinMode(lightPin, OUTPUT);
    }

    void onCommand(const std::string &payload)
    {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, payload.c_str());
        const bool on = doc["on"].as<bool>();
        if (on)
        {
            turnOn();
        }
        else
        {
            turnOff();
        }
        DynamicJsonDocument statusDoc(256);
        statusDoc["isOn"] = on;
        std::string statusMessage = "";
        serializeJson(statusDoc, statusMessage);
        publishStatus(statusMessage);
    }

    void turnOn()
    {
        digitalWrite(lightPin, HIGH);
    }

    void turnOff()
    {
        digitalWrite(lightPin, LOW);
    }
};

#endif