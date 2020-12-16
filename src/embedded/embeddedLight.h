#ifndef EMBEDDED_LIGHT_H
#define EMBEDDED_LIGHT_H

#include "embedded.h"
#include <ArduinoJson.h>

class EmbeddedLight : public EmbeddedDevice
{
private:
    const int lightPin;

public:
    EmbeddedLight(PubSubClient &mqttClient, int lightPin) : EmbeddedDevice(mqttClient), lightPin(lightPin)
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