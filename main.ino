#include <LovyanGFX.hpp>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "RobotEyes.h"

// --- CONFIG ---
#define RX_PIN 18 // Camera RX
#define TX_PIN 17 // Camera TX

// Hardware Objects
Adafruit_MPU6050 mpu;
RobotEyes eyes;

// Display Driver Setup
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

// Global Targets
float camTargetX = 0.0;
float camTargetY = 0.0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Camera Link

  display.init();
  display.setBrightness(128);
  display.setRotation(2);
  sprite.setColorDepth(1);
  sprite.createSprite(128, 64);
  eyes.init();
  
  // Initialize MPU6050 on the shared I2C bus
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip! Check wiring.");
  } else {
    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  
  Serial.println("System Ready. Sensory Integration Active.");
}

void loop() {
  // 1. Read Camera Data (Vision)
  processCameraData();

  // 2. Read MPU6050 Data (Balance/Vestibular)
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // --- PHYSICS ENGINE: TILT & SHAKE ---
  
  // A. Shake Detection (Total Acceleration Vector)
  // Gravity is normally ~9.8. If the total force is much higher, we are being shaken!
  float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  
  if (totalAccel > 15.0) { // 15.0 m/s^2 is a good "vigorous shake" threshold
      if (eyes.getEmotion() != ANGRY) {
          eyes.setEmotion(ANGRY);
          Serial.println("Stop shaking me!");
      }
  }

  // B. Gaze Stabilization (Counter-steering the eyes)
  // When the robot tilts left (X accel changes), the eyes look right to stay level
  float tiltOffsetX = a.acceleration.y * 0.15; // Tweak 0.15 for sensitivity
  float tiltOffsetY = a.acceleration.x * 0.15;

  // Combine Camera Target with Tilt Offset
  float finalEyeX = camTargetX - tiltOffsetX;
  float finalEyeY = camTargetY + tiltOffsetY;

  // 3. Render Output
  eyes.lookAt(finalEyeX, finalEyeY);
  eyes.update();
  eyes.draw(&sprite);
  sprite.pushSprite(0, 0);
}

// Robust Camera Parsing Function
void processCameraData() {
  static unsigned long lastFaceTime = 0;
  static String packetBuffer = "";
  
  while (Serial1.available()) {
    char c = Serial1.read();
    
    if (c == '\n') {
      packetBuffer.trim();
      
      if (packetBuffer.startsWith("F:")) {
        int commaIndex = packetBuffer.indexOf(',');
        if (commaIndex > 0) {
          String xStr = packetBuffer.substring(2, commaIndex);
          String yStr = packetBuffer.substring(commaIndex + 1);
          
          // Update global camera targets
          camTargetX = xStr.toInt() / 100.0; 
          camTargetY = yStr.toInt() / 100.0;
          lastFaceTime = millis();
          
          if (eyes.getEmotion() == SLEEPY || eyes.getEmotion() == INNOCENT) {
             eyes.setEmotion(NEUTRAL);
          }
        }
      }
      packetBuffer = ""; 
    } else {
      packetBuffer += c;
      if (packetBuffer.length() > 50) packetBuffer = ""; // Safety clear
    }
  }

  // Timeout: If no movement seen for 5 seconds, get sleepy/bored
  if (millis() - lastFaceTime > 5000) {
     camTargetX = 0; // Look forward
     camTargetY = 0;
     if (eyes.getEmotion() == NEUTRAL) {
        eyes.setEmotion(SLEEPY); // Robot falls asleep if nothing moves!
     }
  }
}