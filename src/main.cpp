#include "HomePiDevice.h"
#include "mqtt/MqttLight.h"

auto wifiClient = std::make_shared<WiFiClient>();
auto mqttClient = std::make_shared<PubSubClient>(*wifiClient);
auto light = std::make_shared<MqttLight>("light:1", mqttClient, 0);
HomePiDevice hpDevice(light);

void setup()
{
  Serial.begin(115200);
  hpDevice.setup();
}

void loop()
{
  hpDevice.loop();
}
