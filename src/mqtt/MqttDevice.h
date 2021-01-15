#ifndef MQTT_DEVICE_H
#define MQTT_DEVICE_H
#include <memory>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MqttDevice
{
private:
    const std::string deviceId;
    const std::shared_ptr<PubSubClient> mqttClient;
    std::shared_ptr<std::string> mqttBroker;
    std::shared_ptr<std::string> commandTopic;
    std::shared_ptr<std::string> statusTopic;
    long lastMqttReconnectTime;

    void reconnectedMqtt()
    {
        long currentTime = millis();
        if (currentTime > lastMqttReconnectTime + 5000)
        {
            lastMqttReconnectTime = currentTime;
            String clientId = "HomePiClient-";
            clientId += String(random(0xffff), HEX);
            Serial.println("Trying to connect with clientId=" + clientId);
            if (mqttClient->connect(clientId.c_str()))
            {
                Serial.println("Successfully connected to MQTT broker, subscribing to all topic...");
                if (commandTopic != nullptr)
                {
                    mqttClient->subscribe(commandTopic->c_str());
                    onStatusTopicSubscribed();
                }
            }
            else
            {
                Serial.print("Failed to connect to MQTT broker, rc=");
                Serial.print(mqttClient->state());
                Serial.println(". Try again in 5 seconds...");
            }
        }
    }

public:
    MqttDevice(const std::string &deviceId, const std::shared_ptr<PubSubClient> &mqttClient) : deviceId(deviceId), mqttClient(mqttClient)
    {
        mqttBroker = nullptr;
        commandTopic = nullptr;
        statusTopic = nullptr;
        lastMqttReconnectTime = 0;
        mqttClient->setCallback([this](char *topic, byte *payload, unsigned int length) {
            std::string payloadStr = "";
            payloadStr.reserve(length);
            for (size_t i = 0; i < length; i++)
            {
                payloadStr.push_back((char)payload[i]);
            }
            DynamicJsonDocument payloadJson(256);
            deserializeJson(payloadJson, payloadStr.c_str());
            const std::string deviceId = payloadJson["deviceId"].as<std::string>();
            if (getDeviceId() == deviceId)
            {
                onCommand(payloadJson);
            }
        });
    }

    const std::string getDeviceId() { return deviceId; }

    void setMqttBroker(const std::string &mqttBroker, const int &port)
    {
        this->mqttBroker = std::make_shared<std::string>(mqttBroker);
        mqttClient->setServer(this->mqttBroker->c_str(), port);
    }

    void loop()
    {
        if (mqttBroker == nullptr)
        {
            return;
        }
        if (!mqttClient->connected())
        {
            reconnectedMqtt();
        }
        if (mqttClient->connected())
        {
            mqttClient->loop();
        }
    }

    void setCommandTopic(const std::string &topic)
    {
        if (mqttClient->connected() && commandTopic != nullptr)
        {
            mqttClient->unsubscribe(commandTopic->c_str());
        }
        commandTopic = std::make_shared<std::string>(topic);
        if (mqttClient->connected())
        {
            mqttClient->subscribe(commandTopic->c_str());
        }
    }

    void setStatusTopic(const std::string &topic)
    {
        statusTopic = std::make_shared<std::string>(topic);
    }

    void publishStatus(const std::string &fieldName, const std::string &fieldValue)
    {
        if (statusTopic == nullptr)
        {
            return;
        }
        DynamicJsonDocument payload(256);
        payload["deviceId"] = deviceId;
        payload["fieldName"] = fieldName;
        payload["fieldValue"] = fieldValue;
        std::string payloadStr = "";
        serializeJson(payload, payloadStr);
        mqttClient->publish(statusTopic->c_str(), payloadStr.c_str());
    }

    void virtual onCommand(const JsonDocument &payload) = 0;

    void virtual onStatusTopicSubscribed() = 0;
};

#endif