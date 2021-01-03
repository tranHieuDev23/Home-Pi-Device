#ifndef HOME_PI_DEVICE_H
#define HOME_PI_DEVICE_H

#include <memory>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <PubSubClient.h>
#include "mqtt/MqttLight.h"
#include "utils/WifiUtils.h"

#define BT_BUFFER_MAX_SIZE 4096
#define BT_BUFFER_TIMEOUT 3000
#define TIMEOUT 30000

class HomePiLight
{
private:
    std::string btBuffer;
    long lastBtReadTime;
    const std::string deviceId;
    std::shared_ptr<WiFiClient> wifiClient;
    std::shared_ptr<BluetoothSerial> btClient;
    std::shared_ptr<PubSubClient> mqttClient;
    std::shared_ptr<MqttLight> light;

    void readBluetooth()
    {
        if (!btClient->available())
        {
            return;
        }
        long currentTime = millis();
        if (currentTime - lastBtReadTime > BT_BUFFER_TIMEOUT)
        {
            btBuffer.clear();
        }
        lastBtReadTime = currentTime;
        while (btClient->available())
        {
            char nextCharacter = btClient->read();
            if (nextCharacter == '\n')
            {
                onBluetoothMessage(btBuffer);
                btBuffer.clear();
            }
            else if (btBuffer.size() < BT_BUFFER_MAX_SIZE)
            {
                btBuffer.push_back(nextCharacter);
            }
        }
    }

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
        if (action == "wifiStatus")
        {
            responseWifiStatus(reqId, true);
            return;
        }
        if (action == "scanWifi")
        {
            scanWifi(reqId, true);
            return;
        }
        bool success = false;
        if (action == "connectWifi")
        {
            const std::string ssid = doc["ssid"].as<std::string>();
            const std::string psk = doc["psk"].as<std::string>();
            success = connectWifi(ssid, psk, TIMEOUT);
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

    void responseWifiStatus(const std::string &reqId, const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["reqId"] = reqId;
        doc["success"] = success;
        doc["connected"] = (wifiClient->connected() != 0);
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    void scanWifi(const std::string &reqId, const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["reqId"] = reqId;
        doc["success"] = success;
        auto results = scanWiFiNetworks();
        JsonArray networks = doc.createNestedArray("networks");
        for (const auto &item : results)
        {
            Serial.print(item.ssid.c_str());
            Serial.print(" ");
            Serial.println(item.open);
            JsonObject itemObj = networks.createNestedObject();
            itemObj["ssid"] = item.ssid;
            itemObj["open"] = item.open;
        }
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    bool connectWifi(const std::string &ssid, const std::string &psk, long timeout)
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
        btBuffer = "";
        lastBtReadTime = 0;
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
        readBluetooth();
        light->loop();
    }
};

#endif