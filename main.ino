#include <LovyanGFX.hpp>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "RobotEyes.h"

#define RX_PIN 18
#define TX_PIN 17

Adafruit_MPU6050 mpu;
RobotEyes eyes;

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_SH110x _panel_instance;
  lgfx::Bus_I2C _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.i2c_port = 0;
      cfg.freq_write = 400000;
      cfg.pin_sda = 4;
      cfg.pin_scl = 5;
      cfg.i2c_addr = 0x3C;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.panel_width = 128;
      cfg.panel_height = 64;
      cfg.offset_x = 2;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX display;
LGFX_Sprite sprite(&display);

// Interaction Logic
unsigned long lastInteractionTime = 0;
unsigned long emotionOverrideTimer = 0;
bool hasEmotionOverride = false; // True when Angry/Dizzy/Wakeup is playing

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  display.init();
  display.setBrightness(128);
  display.setRotation(2);
  sprite.setColorDepth(1);
  sprite.createSprite(128, 64);
  eyes.init();

  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050");
  }
  else
  {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  lastInteractionTime = millis();
}

void loop()
{
  bool cameraDetected = false;
  bool physicallyMoved = false;

  // 1. Process Camera (Controls Pupil Only - does NOT wake from sleep)
  cameraDetected = processCameraData();

  // 2. Process MPU6050 (Controls Base Offset & Physical Reactions)
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // MPU Metrics
  float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  float totalGyro = sqrt(pow(g.gyro.x, 2) + pow(g.gyro.y, 2) + pow(g.gyro.z, 2));

  // Gaze Stabilization: Shift the ENTIRE eye in the opposite direction of the tilt
  eyes.setEyeOffset(-a.acceleration.x * 1.5, a.acceleration.y * 1.5);

  // Check Physical Interactions (MPU only)
  if (totalAccel > 22.0)
  { // Big Spike (Shake)
    eyes.setEmotion(ANGRY);
    emotionOverrideTimer = millis();
    hasEmotionOverride = true;
    physicallyMoved = true;
  }
  else if (totalGyro > 5.0)
  { // Fast Spinning (Dizzy)
    eyes.setEmotion(DIZZY);
    emotionOverrideTimer = millis();
    hasEmotionOverride = true;
    physicallyMoved = true;
  }
  else if (abs(a.acceleration.x) > 2.0 || abs(a.acceleration.y) > 2.0)
  {
    // Gentle physical movement
    physicallyMoved = true;
  }

  // --- INTERACTION STATE MACHINE ---

  // A. Waking Up from Sleep - ONLY physical MPU movement can wake
  if (physicallyMoved)
  {
    if (eyes.getEmotion() == SLEEPY || eyes.getEmotion() == ASLEEP)
    {
      eyes.setEmotion(WAKEUP);
      emotionOverrideTimer = millis();
      hasEmotionOverride = true;
    }
    lastInteractionTime = millis();
  }

  // B. Camera keeps idle timer alive when awake (cannot wake from sleep)
  Emotion curEmotion = eyes.getEmotion();
  if (cameraDetected && curEmotion != SLEEPY && curEmotion != ASLEEP)
  {
    lastInteractionTime = millis();
  }

  // C. Returning to Idle (Neutral Blink)
  if (hasEmotionOverride && (millis() - emotionOverrideTimer > 3000))
  {
    // 3 seconds have passed since the special emotion started
    eyes.setEmotion(NEUTRAL);
    hasEmotionOverride = false;
  }

  // D. Getting Bored / Going to Sleep
  if (!hasEmotionOverride)
  {
    unsigned long idleTime = millis() - lastInteractionTime;

    if (idleTime > 20000)
    {
      // 20 Seconds idle: Deep Sleep
      if (eyes.getEmotion() != ASLEEP)
        eyes.setEmotion(ASLEEP);
    }
    else if (idleTime > 10000)
    {
      // 10 Seconds idle: Start getting sleepy
      if (eyes.getEmotion() != SLEEPY)
        eyes.setEmotion(SLEEPY);
    }
  }

  // Render Frame
  eyes.update();
  eyes.draw(&sprite);
  sprite.pushSprite(0, 0);
}

// Returns TRUE if a face/motion was actively detected this frame
bool processCameraData()
{
  bool detected = false;
  static String packetBuffer = "";

  while (Serial1.available())
  {
    char c = Serial1.read();
    if (c == '\n')
    {
      packetBuffer.trim();
      if (packetBuffer.startsWith("F:"))
      {
        int commaIndex = packetBuffer.indexOf(',');
        if (commaIndex > 0)
        {
          String xStr = packetBuffer.substring(2, commaIndex);
          String yStr = packetBuffer.substring(commaIndex + 1);

          float x = xStr.toInt() / 100.0;
          float y = yStr.toInt() / 100.0;

          eyes.lookAt(x, y); // Set pupil target
          detected = true;
        }
      }
      packetBuffer = "";
    }
    else
    {
      packetBuffer += c;
      if (packetBuffer.length() > 50)
        packetBuffer = "";
    }
  }

  // Center pupils if nothing is seen
  if (!detected && millis() % 100 == 0)
  {
    // Very slowly drift pupils back to center when blind
    eyes.lookAt(0, 0);
  }

  return detected;
}