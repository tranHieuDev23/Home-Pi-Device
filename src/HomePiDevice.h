#ifndef HOME_PI_DEVICE_H
#define HOME_PI_DEVICE_H

#include <memory>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include "mqtt/MqttDevice.h"
#include "utils/WifiUtils.h"

#define HOME_PI_CLOUD_URL "http://1ecea3c289c9.ngrok.io"
#define HOME_PI_CLOUD_REGISTER_API HOME_PI_CLOUD_URL "/api/home-control/validate-device"
#define BT_BUFFER_MAX_SIZE 4096
#define BT_BUFFER_TIMEOUT 3000
#define TIMEOUT 30000

class HomePiDevice
{
private:
    std::string btBuffer;
    long lastBtReadTime;
    std::shared_ptr<BluetoothSerial> btClient;
    std::shared_ptr<MqttDevice> device;

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
        const std::string action = doc["action"].as<std::string>();
        if (action == "getId")
        {
            responseDeviceId(true);
            return;
        }
        if (action == "wifiStatus")
        {
            responseWifiStatus(true);
            return;
        }
        if (action == "scanWifi")
        {
            scanWifi(true);
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
            const std::string token = doc["token"].as<std::string>();
            success = setRegistrationData(token);
        }
        responseRequestSuccess(success);
    }

    void responseRequestSuccess(const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["success"] = success;
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    void responseDeviceId(const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["success"] = success;
        doc["deviceId"] = device->getDeviceId();
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    void responseWifiStatus(const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["success"] = success;
        doc["connected"] = WiFi.isConnected();
        std::string response = "";
        serializeJson(doc, response);
        btClient->println(response.c_str());
    }

    void scanWifi(const bool &success)
    {
        DynamicJsonDocument doc(1024);
        doc["success"] = success;
        auto results = scanWiFiNetworks();
        JsonArray networks = doc.createNestedArray("networks");
        for (const auto &item : results)
        {
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

    bool setRegistrationData(const std::string &token)
    {
        Serial.println("Setting registration data");

        DynamicJsonDocument requestJson(1024);
        requestJson["token"] = token;
        String requestStr = "";
        serializeJson(requestJson, requestStr);

        HTTPClient httpClient;
        httpClient.setReuse(false);
        if (!httpClient.begin(HOME_PI_CLOUD_REGISTER_API))
        {
            Serial.println("Failed to establish HTTP connection to Home Pi Cloud");
            return false;
        }
        Serial.println("Successfully establish HTTP connection to Home Pi Cloud");

        httpClient.addHeader("Content-Type", "application/json");
        int responseCode = httpClient.POST(requestStr);
        if (responseCode > 0 && responseCode != HTTP_CODE_OK)
        {
            httpClient.end();
            Serial.print("Failed to register device, error code = ");
            Serial.println(responseCode);
            return false;
        }

        DynamicJsonDocument responseJson(1024);
        String responseStr = httpClient.getString();
        deserializeJson(responseJson, responseStr);
        const std::string broker = responseJson["broker"].as<std::string>();
        const int port = responseJson["port"].as<int>();
        const std::string commandTopic = responseJson["commandTopic"].as<std::string>();
        const std::string statusTopic = responseJson["statusTopic"].as<std::string>();
        httpClient.end();

        device->setMqttBroker(broker, port);
        device->setCommandTopic(commandTopic);
        device->setStatusTopic(statusTopic);

        Serial.println("Register device successfully");
        return true;
    }

public:
    HomePiDevice(std::shared_ptr<MqttDevice> device) : device(device)
    {
        btBuffer = "";
        lastBtReadTime = 0;
        btClient = std::make_shared<BluetoothSerial>();
    }

    void setup()
    {
        btClient->begin("Home Pi Light");
    }

    void loop()
    {
        readBluetooth();
        device->loop();
    }
};

#endif