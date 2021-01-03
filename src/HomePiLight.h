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
    const std::string deviceId;
    std::shared_ptr<WiFiClient> wifiClient;
    std::shared_ptr<BluetoothSerial> btClient;
    std::shared_ptr<PubSubClient> mqttClient;
    std::shared_ptr<MqttLight> light;

    void onBluetoothMessage(const std::string &message)
    {
        Serial.println(("bluetooth message " + message + "").c_str());

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, message.c_str());
        const std::string reqId = doc["reqId"].as<std::string>();
        const std::string action = doc["action"].as<std::string>();
        if (action == "getId")
        {
            responseDeviceId(reqId, true);
            return;
        }
        bool success = false;
        if (action == "connectWifi")
        {
            const std::string ssid = doc["ssid"].as<std::string>();
            const std::string psk = doc["psk"].as<std::string>();
            success = connectWiFi(ssid, psk, TIMEOUT);
        }
        if (action == "register")
        {
            const std::string commandTopic = doc["commandTopic"].as<std::string>();
            const std::string statusTopic = doc["statusTopic"].as<std::string>();
            const std::string token = doc["token"].as<std::string>();
            success = setRegistrationData(commandTopic, statusTopic, token);
        }
        responseRequestSuccess(reqId, success);
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

    void responseDeviceId(const std::string &reqId, const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["reqId"] = reqId;
        doc["success"] = success;
        doc["deviceId"] = deviceId;
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

    bool setRegistrationData(const std::string &commandTopic, const std::string &statusTopic, const std::string &token)
    {
        Serial.println("Setting registration data");
        light->setCommandTopic(commandTopic);
        light->setStatusTopic(statusTopic);
        return true;
    }

public:
    HomePiLight(const std::string &deviceId, int lightPin) : deviceId(deviceId)
    {
        wifiClient = std::make_shared<WiFiClient>();
        btClient = std::make_shared<BluetoothSerial>();
        mqttClient = std::make_shared<PubSubClient>(*wifiClient);
        light = std::make_shared<MqttLight>(mqttClient, lightPin);
    }

    void setup()
    {
        btClient->begin("Home Pi Light");
        light->setMqttBroker("broker.hivemq.com", 1883);
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