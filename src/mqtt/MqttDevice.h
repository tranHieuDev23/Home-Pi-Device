#ifndef MQTT_DEVICE_H
#define MQTT_DEVICE_H
#include <memory>
#include <PubSubClient.h>

class MqttDevice
{
private:
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
    MqttDevice(const std::shared_ptr<PubSubClient> &mqttClient) : mqttClient(mqttClient)
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
            onCommand(payloadStr);
        });
    }

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

    void publishStatus(const std::string &payload)
    {
        if (statusTopic == nullptr)
        {
            return;
        }
        mqttClient->publish(statusTopic->c_str(), payload.c_str());
    }

    void virtual onCommand(const std::string &payload) = 0;
};

#endif