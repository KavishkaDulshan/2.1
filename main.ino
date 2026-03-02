#include <LovyanGFX.hpp>
#include "RobotEyes.h"

// --- CONFIG ---
#define RX_PIN 18 // Connect to ESP32-CAM U0T
#define TX_PIN 17 // Connect to ESP32-CAM U0R

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_SH110x _panel_instance;
  lgfx::Bus_I2C      _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.i2c_port = 0;
      cfg.freq_write = 400000;
      cfg.pin_sda = 4; cfg.pin_scl = 5; cfg.i2c_addr = 0x3C;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.panel_width = 128; cfg.panel_height = 64;
      cfg.offset_x = 2;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX display;
LGFX_Sprite sprite(&display);
RobotEyes eyes;

void setup() {
  Serial.begin(115200); // Debug USB
  
  // Init Camera Link
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  display.init();
  display.setBrightness(128);
  display.setRotation(2);
  sprite.setColorDepth(1);
  sprite.createSprite(128, 64);
  eyes.init();
  
  Serial.println("System Ready. Waiting for face...");
}

void loop() {
  // 1. Read Face Data from Camera
  processCameraData();

  // 2. Physics & Drawing
  eyes.update();
  eyes.draw(&sprite);
  sprite.pushSprite(0, 0);
}

// Function to read data from ESP32-CAM
void processCameraData() {
  static unsigned long lastFaceTime = 0;
  
  while (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    data.trim();
    
    // DEBUG PRINT: Show EXACTLY what the S3 is receiving
    if (data.length() > 0) {
       Serial.print("Received: ");
       Serial.println(data);
    }
    
    // Packet Format: "F:x,y" (e.g., F:-50,20)
    if (data.startsWith("F:")) {
      int commaIndex = data.indexOf(',');
      if (commaIndex > 0) {
        String xStr = data.substring(2, commaIndex);
        String yStr = data.substring(commaIndex + 1);
        
        // Map -100/100 to float -1.0/1.0
        float x = xStr.toInt() / 100.0; 
        float y = yStr.toInt() / 100.0;
        
        eyes.lookAt(x, y);
        lastFaceTime = millis();
        
        // If we were bored/sleeping, wake up!
        if (eyes.getEmotion() == SLEEPY || eyes.getEmotion() == INNOCENT) {
           eyes.setEmotion(NEUTRAL);
        }
      }
    }
  }

  // Timeout logic remains the same...
  if (millis() - lastFaceTime > 5000) {
     if (eyes.getEmotion() == NEUTRAL) {
        eyes.setEmotion(INNOCENT); 
     }
  }
}