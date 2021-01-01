#ifndef MQTT_LIGHT_H
#define MQTT_LIGHT_H

#include "MqttDevice.h"
#include <ArduinoJson.h>

class MqttLight : public MqttDevice
{
private:
    const uint8_t lightPin;
    std::string statusTopic;

public:
    MqttLight(PubSubClient &mqttClient, uint8_t lightPin) : MqttDevice(mqttClient), lightPin(lightPin)
    {
        pinMode(lightPin, OUTPUT);
    }

    void setStatusTopic(const std::string &topic)
    {
        statusTopic = topic;
    }

    void subscribeCommandTopic(const std::string &topic)
    {
        subscribeTopic(SubscribeHandler(topic, [this](const std::string &payload) {
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
            publishTopic(statusTopic, statusMessage);
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