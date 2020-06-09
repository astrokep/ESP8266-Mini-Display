
#include "MiniDisplay.h"
#include "secrets.h"

MiniDisplayProperties mdp;

void setup() {
  Serial.begin(9600);
  initMiniDisplay(&mdp);
  initScreens(&mdp);
  initDisplay(&mdp, 128, 64, 0x3C, -1);
  initWifi(&mdp, SSID, PASSWD);
  initNTPTimeClient(&mdp, -6);
  int encoder_pins[2] = {12, 13};
  registerIO(&mdp, 14, encoder_pins);
}

void loop() {
  updateTime(&mdp);
  updateDisplay(&mdp);
}
