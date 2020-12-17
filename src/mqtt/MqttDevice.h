#ifndef MQTT_DEVICE_H
#define MQTT_DEVICE_H
#include <vector>
#include <memory>
#include <functional>
#include <PubSubClient.h>

typedef std::shared_ptr<std::string> strp;
typedef std::function<void(const std::string)> SubHandleFunc;
typedef std::function<strp()> PubMessageFunc;

class SubscribeHandler
{
public:
    const std::string topic;
    const SubHandleFunc onMessage;

    SubscribeHandler(std::string topic, SubHandleFunc onMessage)
        : topic(topic), onMessage(onMessage) {}

    const void callOnMessage(const std::string &payload) const
    {
        onMessage(payload);
    }
};

class IntervalPublish
{
public:
    const std::string topic;
    const long publishInterval;
    const PubMessageFunc onPublishTime;

    IntervalPublish(std::string topic, long publishInterval, PubMessageFunc onPublishTime)
        : topic(topic), publishInterval(publishInterval), onPublishTime(onPublishTime) {}

    strp callOnPublishTime() const
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
    std::shared_ptr<std::string> mqttBroker;
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
            }
        }
    }

    void subscribeAllTopic()
    {
        for (const auto &item : subscribedTopics)
        {

            mqttClient.subscribe(item.topic.c_str());
        }
    }

    void onMessage(const std::string &topic, const std::string &payload)
    {
        for (const auto &item : subscribedTopics)
        {
            if (item.topic == topic)
            {
                item.callOnMessage(payload);
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
        mqttBroker = nullptr;
        lastMqttReconnectTime = 0;
    }

    void connectMqttBroker(const std::string &broker, uint16_t port)
    {
        mqttBroker = std::make_shared<std::string>(std::string(broker));
        mqttClient.setServer(mqttBroker->c_str(), port);
        mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length) {
            const std::string topicStr(topic);
            std::string payloadStr = "";
            payloadStr.reserve(length);
            for (size_t i = 0; i < length; i++)
            {
                payloadStr.push_back((char)payload[i]);
            }
            onMessage(topicStr, payloadStr);
        });
    }

    void loop()
    {
        if (mqttBroker == nullptr)
        {
            return;
        }
        if (!mqttClient.connected())
        {
            reconnectedMqtt();
        }
        if (mqttClient.connected())
        {
            mqttClient.loop();
            long currentTime = millis();
            for (size_t i = 0; i < intervalPublishTopics.size(); i++)
            {
                const IntervalPublish &item = intervalPublishTopics[i];
                if (currentTime > item.publishInterval + lastPublishTimes[i])
                {
                    strp message = item.callOnPublishTime();
                    if (message == nullptr)
                    {
                        continue;
                    }
                    publishTopic(item.topic, *message);
                    lastPublishTimes[i] = currentTime;
                }
            }
        }
    }

    int subscribeTopic(const SubscribeHandler &item)
    {
        subscribedTopics.push_back(item);
        if (mqttClient.connected())
        {
            mqttClient.subscribe(item.topic.c_str());
        }
        return subscribedTopics.size();
    }

    void publishTopic(const std::string &topic, const std::string &payload)
    {
        mqttClient.publish(topic.c_str(), payload.c_str());
    }

    int addIntervalPublish(const IntervalPublish &item)
    {
        intervalPublishTopics.push_back(item);
        lastPublishTimes.push_back(-1);
        return intervalPublishTopics.size();
    }
};

#endif