#include "HomePiLight.h"

HomePiLight light(0);

void setup()
{
  Serial.begin(115200);
  light.setup();
}

void loop()
{
  light.loop();
}
