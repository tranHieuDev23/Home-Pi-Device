#include <Arduino.h>
#include <WiFi.h>
#include "embedded/embeddedLight.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
EmbeddedLight light(mqttClient, 0);
const char *ssid = "Wifi Hieu 2.4G";
const char *psk = "tranminhhieu23";
const char *broker = "broker.hivemq.com";
char *toggleTopic = "hieutm/light/toggle";

void connectWifi(const char *&ssid, const char *&psk)
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psk);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  connectWifi(ssid, psk);
  Serial.println("Wifi connect successful");
  light.connectMqttBroker(broker, 1883);
  Serial.println("MQTT connect successful");
  light.subscribeToggleTopic(toggleTopic);
  Serial.println("Topic subscribe successful");
}

void loop()
{
  light.loop();
}
