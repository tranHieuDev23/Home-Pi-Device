#include "HomePiLight.h"

HomePiLight light("light:12345", 0);

void setup()
{
  Serial.begin(115200);
  light.setup();
}

void loop()
{
  light.loop();
}
