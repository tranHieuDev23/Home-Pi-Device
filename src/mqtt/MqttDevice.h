#ifndef MQTT_DEVICE_H
#define MQTT_DEVICE_H
#include <vector>
#include <functional>
#include <PubSubClient.h>

typedef std::function<void(char *, unsigned int)> SubHandleFunc;
typedef std::function<char *()> PubMessageFunc;

class SubscribeHandler
{
public:
    const char *topic;
    const SubHandleFunc onMessage;

    SubscribeHandler(char *topic, SubHandleFunc onMessage)
        : topic(topic), onMessage(onMessage) {}

    const void callOnMessage(char *payload, unsigned int length) const
    {
        onMessage(payload, length);
    }
};

class IntervalPublish
{
public:
    const char *topic;
    const long publishInterval;
    const PubMessageFunc onPublishTime;

    IntervalPublish(char *topic, long publishInterval, PubMessageFunc onPublishTime)
        : topic(topic), publishInterval(publishInterval), onPublishTime(onPublishTime) {}

    char *callOnPublishTime() const
    {
        return onPublishTime();
    }
};

class MqttDevice
{
private:
    PubSubClient mqttClient;
    std::vector<SubscribeHandler> subscribedTopics;
    std::vector<IntervalPublish> intervalPublishTopics;
    std::vector<long> lastPublishTimes;

    void reconnectedMqtt()
    {
        while (!mqttClient.connected())
        {
            String clientId = "HomePiClient-";
            clientId += String(random(0xffff), HEX);
            Serial.println("Trying to connect with clientId=" + clientId);
            if (mqttClient.connect(clientId.c_str()))
            {
                Serial.print("Successfully connected to MQTT broker, subscribing to all topic...");
                subscribeAllTopic();
            }
            else
            {
                Serial.print("Failed to connect to MQTT broker, rc=");
                Serial.print(mqttClient.state());
                Serial.println(". Try again in 5 seconds...");
                delay(5000);
            }
        }
    }

    void subscribeAllTopic()
    {
        for (const auto &item : subscribedTopics)
        {
            mqttClient.subscribe(item.topic);
        }
    }

    void onMessage(char *topic, byte *payload, unsigned int length)
    {
        for (const auto &item : subscribedTopics)
        {
            if (strcmp(item.topic, topic) == 0)
            {
                item.callOnMessage((char *)payload, length);
            }
        }
    }

public:
    MqttDevice(PubSubClient &_mqttClient)
    {
        mqttClient = _mqttClient;
        subscribedTopics = std::vector<SubscribeHandler>();
        intervalPublishTopics = std::vector<IntervalPublish>();
        lastPublishTimes = std::vector<long>();
    }

    void connectMqttBroker(const char *broker, uint16_t port)
    {
        mqttClient.setServer(broker, port);
        mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length) {
            onMessage(topic, payload, length);
        });
    }

    void loop()
    {
        if (!mqttClient.connected())
        {
            reconnectedMqtt();
        }
        mqttClient.loop();
        long currentTime = millis();
        for (size_t i = 0; i < intervalPublishTopics.size(); i++)
        {
            const IntervalPublish &item = intervalPublishTopics[i];
            if (currentTime > item.publishInterval + lastPublishTimes[i])
            {
                char *message = item.callOnPublishTime();
                if (message == nullptr)
                {
                    continue;
                }
                publishTopic(item.topic, message);
                lastPublishTimes[i] = currentTime;
            }
        }
    }

    void subscribeTopic(const SubscribeHandler &item)
    {
        subscribedTopics.push_back(item);
        if (mqttClient.connected())
        {
            mqttClient.subscribe(item.topic);
        }
    }

    void publishTopic(const char *topic, const char *payload)
    {
        mqttClient.publish(topic, payload);
    }

    void addIntervalPublish(const IntervalPublish &item)
    {
        intervalPublishTopics.push_back(item);
        lastPublishTimes.push_back(-1);
    }
};

#endif