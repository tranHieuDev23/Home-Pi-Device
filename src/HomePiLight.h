#ifndef HOME_PI_DEVICE_H
#define HOME_PI_DEVICE_H

#include <memory>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <PubSubClient.h>
#include "mqtt/MqttLight.h"

#define TIMEOUT 30000

class HomePiLight
{
private:
    std::shared_ptr<WiFiClient> wifiClient;
    std::shared_ptr<BluetoothSerial> btClient;
    std::shared_ptr<PubSubClient> mqttClient;
    std::shared_ptr<MqttLight> light;

    void onBluetoothMessage(const std::string &message)
    {
        Serial.println(("onBluetoothMessage(\"" + message + ")").c_str());

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, message.c_str());
        const std::string reqId = doc["reqId"].as<std::string>();
        const std::string action = doc["action"].as<std::string>();
        if (action == "connectWifi")
        {
            const std::string ssid = doc["ssid"].as<std::string>();
            const std::string psk = doc["psk"].as<std::string>();
            responseRequestSuccess(reqId, connectWiFi(ssid, psk, TIMEOUT));
        }
        if (action == "registerBroker")
        {
            const std::string broker = doc["broker"].as<std::string>();
            const int port = doc["port"].as<int>();
            responseRequestSuccess(reqId, connectMqttBroker(broker, port, TIMEOUT));
        }
        if (action == "registerTopic")
        {
            const std::string topic = doc["topic"].as<std::string>();
            responseRequestSuccess(reqId, subscribeToggleTopic(topic, TIMEOUT));
        }
    }

    void responseRequestSuccess(const std::string &reqId, const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["reqId"] = reqId;
        doc["success"] = success;
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    bool connectWiFi(const std::string &ssid, const std::string &psk, long timeout)
    {
        Serial.print("Trying to connect to ssid=");
        Serial.print(ssid.c_str());
        Serial.print(" with psk=");
        Serial.println(psk.c_str());
        delay(10);

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), psk.c_str());
        long endTime = millis() + timeout;
        while ((WiFi.status() != WL_CONNECTED) && (millis() < endTime))
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nConnected to WiFi successfully");
            return true;
        }
        else
        {
            Serial.println("\nUnable to connected to WiFi");
            return false;
        }
    }

    bool connectMqttBroker(const std::string &broker, const int &port, long timeout)
    {
        Serial.print("Connecting to MQTT broker=");
        Serial.print(broker.c_str());
        Serial.print(", port=");
        Serial.println(port);
        light->connectMqttBroker(broker, port);
        return true;
    }

    bool subscribeToggleTopic(const std::string &topic, long timeout)
    {
        Serial.print("Subscribing to MQTT topic=");
        Serial.print(topic.c_str());
        light->subscribeToggleTopic(topic);
        return true;
    }

public:
    HomePiLight(int lightPin)
    {
        wifiClient = std::make_shared<WiFiClient>(WiFiClient());
        btClient = std::make_shared<BluetoothSerial>(BluetoothSerial());
        mqttClient = std::make_shared<PubSubClient>(PubSubClient(*wifiClient));
        light = std::make_shared<MqttLight>(MqttLight(*mqttClient, lightPin));
    }

    void setup()
    {
        btClient->begin("Home Pi Light");
    }

    void loop()
    {
        if (btClient->available())
        {
            String message = btClient->readStringUntil('\n');
            onBluetoothMessage(message.c_str());
        }
        light->loop();
    }
};

#endif