#include <LovyanGFX.hpp>
#include "RobotEyes.h"

void runDemo();

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_SH110x _panel_instance;
  lgfx::Bus_I2C      _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.i2c_port = 0; cfg.freq_write = 400000;
      cfg.pin_sda = 4; cfg.pin_scl = 5; cfg.i2c_addr = 0x3C;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.panel_width = 128; cfg.panel_height = 64; cfg.offset_x = 2;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX display;
LGFX_Sprite sprite(&display);
RobotEyes eyes;

void setup() {
  Serial.begin(115200);
  display.init();
  display.setBrightness(128);
  display.setRotation(2);
  sprite.setColorDepth(1);
  sprite.createSprite(128, 64);
  eyes.init();
}

void loop() {
  eyes.update();
  eyes.draw(&sprite);
  sprite.pushSprite(0, 0);
  runDemo();
}

void runDemo() {
  static unsigned long lastChange = 0;
  static int state = 0;

  // Give SLEEPY more time so the full droop->close->fight->open cycle plays
  unsigned long stateTime = (state == 2) ? 12000 : 6000;
  if (millis() - lastChange > stateTime) {
    state++;
    if (state > 5) state = 0;
    switch(state) {
      case 0: eyes.setEmotion(NEUTRAL);  Serial.println("NEUTRAL");   break;
      case 1: eyes.setEmotion(HAPPY);    Serial.println("HAPPY");     break;
      case 2: eyes.setEmotion(SLEEPY);   Serial.println("SLEEPY");    break;
      case 3: eyes.setEmotion(ANGRY);    Serial.println("ANGRY");     break;
      case 4: eyes.setEmotion(SAD);      Serial.println("SAD");       break;
      case 5: eyes.setEmotion(INNOCENT); Serial.println("INNOCENT");  break;
    }
    lastChange = millis();
  }

  // Random look-around: NEUTRAL and ANGRY only
  if (state == 0 || state == 3) {
    static unsigned long lastMove = 0;
    if (millis() - lastMove > 1000) {
      eyes.lookAt(random(-10, 10) / 10.0, random(-10, 10) / 10.0);
      lastMove = millis();
    }
  }

  // INNOCENT: gentle upward glance, very subtle side drift
  if (state == 5) {
    static unsigned long lastGlance = 0;
    if (millis() - lastGlance > 2500) {
      eyes.lookAt(random(-4, 4) / 10.0, -0.5);
      lastGlance = millis();
    }
  }
}
