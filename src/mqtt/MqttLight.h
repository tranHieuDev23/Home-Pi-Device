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

    void subscribeToggleTopic(const std::string &topic)
    {
        subscribeTopic(SubscribeHandler(topic, [this](const std::string &payload) {
            DynamicJsonDocument doc(1024);
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