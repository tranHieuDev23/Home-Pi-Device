#ifndef MQTT_LIGHT_H
#define MQTT_LIGHT_H

#include "MqttDevice.h"
#include <ArduinoJson.h>

class MqttLight : public MqttDevice
{
private:
    const uint8_t lightPin;
    bool isOn;

public:
    MqttLight(const std::string &deviceId, const std::shared_ptr<PubSubClient> &mqttClient, uint8_t lightPin)
        : MqttDevice(deviceId, mqttClient), lightPin(lightPin)
    {
        pinMode(lightPin, OUTPUT);
        isOn = false;
    }

    void onCommand(const JsonDocument &payload)
    {
        const std::string command = payload["command"].as<std::string>();
        if (command == "turnOn")
        {
            turnOn();
        }
        if (command == "turnOff")
        {
            turnOff();
        }
        publishCurrentStatus();
    }

    void onStatusTopicSubscribed()
    {
        publishCurrentStatus();
    }

    void publishCurrentStatus()
    {
        publishStatus("isOn", isOn ? "true" : "false");
    }

    void turnOn()
    {
        digitalWrite(lightPin, HIGH);
        isOn = true;
    }

    void turnOff()
    {
        digitalWrite(lightPin, LOW);
        isOn = false;
    }
};

#endif