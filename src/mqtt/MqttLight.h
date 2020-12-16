#ifndef MQTT_LIGHT_H
#define MQTT_LIGHT_H

#include "MqttDevice.h"
#include <ArduinoJson.h>

class MqttLight : public MqttDevice
{
private:
    const int lightPin;

public:
    MqttLight(PubSubClient &mqttClient, int lightPin) : MqttDevice(mqttClient), lightPin(lightPin)
    {
        pinMode(lightPin, OUTPUT);
    }

    void subscribeToggleTopic(char *topic)
    {
        subscribeTopic(SubscribeHandler(topic, [this](char *payload, unsigned int length) {
            DynamicJsonDocument doc(128);
            deserializeJson(doc, payload);
            const bool on = doc["on"].as<bool>();
            if (on)
            {
                turnOn();
            }
            else
            {
                turnOff();
            }
        }));
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