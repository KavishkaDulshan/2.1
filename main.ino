#include <LovyanGFX.hpp>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "RobotEyes.h"

// --- HARDWARE PINS ---
#define RX_PIN 18     // Camera RX
#define TX_PIN 17     // Camera TX
#define TOUCH_PIN 14  // Capacitive Touch Sensor
#define VIBE_PIN 13   // Vibrator Motor Module

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
      cfg.i2c_port = 0; cfg.freq_write = 400000;
      cfg.pin_sda = 4; cfg.pin_scl = 5; cfg.i2c_addr = 0x3C;
      _bus_instance.config(cfg); _panel_instance.setBus(&_bus_instance);
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

// Interaction Logic
unsigned long lastInteractionTime = 0;
unsigned long emotionOverrideTimer = 0;
bool hasEmotionOverride = false; // True when Angry/Dizzy/Wakeup/Happy is playing

// Touch-to-Happy transition state
bool wakeupFromTouch = false;
unsigned long wakeupTouchTime = 0;

// Long-press INNOCENT state
bool innocentOverride = false;
unsigned long innocentReleaseTime = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); 

  // Initialize Touch and Haptics
  pinMode(TOUCH_PIN, INPUT);
  pinMode(VIBE_PIN, OUTPUT);
  analogWrite(VIBE_PIN, 0); // Ensure motor is off

  display.init(); display.setBrightness(128); display.setRotation(2);
  sprite.setColorDepth(1); sprite.createSprite(128, 64);
  eyes.init();
  
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  
  lastInteractionTime = millis();
}

void loop() {
  // ── 1. COLLECT ALL INPUTS ────────────────────────────────────────────────

  bool cameraDetected = processCameraData(); // sets pupil target; returns true if face seen

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float totalAccel = sqrt(pow(a.acceleration.x,2) + pow(a.acceleration.y,2) + pow(a.acceleration.z,2));
  float totalGyro  = sqrt(pow(g.gyro.x,2)  + pow(g.gyro.y,2)  + pow(g.gyro.z,2));

  eyes.setEyeOffset(-a.acceleration.x * 1.5, a.acceleration.y * 1.5);

  bool physicallyMoved = false;
  bool strongPhysical  = false;

  if (totalAccel > 22.0) {                                        // Hard shake → Angry
      if (!hasEmotionOverride) eyes.setEmotion(ANGRY);
      emotionOverrideTimer = millis();
      hasEmotionOverride   = true;
      physicallyMoved = strongPhysical = true;
  } else if (totalGyro > 5.0) {                                   // Fast spin → Dizzy
      if (!hasEmotionOverride) eyes.setEmotion(DIZZY);
      emotionOverrideTimer = millis();
      hasEmotionOverride   = true;
      physicallyMoved = strongPhysical = true;
  } else if (abs(a.acceleration.x) > 2.0 || abs(a.acceleration.y) > 2.0) {
      physicallyMoved = true;
  }

  bool isTouched = (digitalRead(TOUCH_PIN) == HIGH);
  static bool wasTouched = false;
  static unsigned long touchHoldStart = 0;
  static bool wasLongPress = false;

  Emotion curEmotion = eyes.getEmotion();

  // ── 2. SLEEP / WAKE LOGIC ────────────────────────────────────────────────

  bool isSleeping = (curEmotion == SLEEPY || curEmotion == ASLEEP);

  // Track how long the finger has been held
  if (isTouched) {
      if (touchHoldStart == 0) touchHoldStart = millis();
  } else {
      if (touchHoldStart != 0) {
          // Finger just lifted
          if (innocentOverride) {
              innocentReleaseTime = millis(); // Start 3s post-release timer
          }
          touchHoldStart = 0;
          wasLongPress   = false;
      }
      wakeupFromTouch = false;   // finger lifted – clear the transition flag
  }

  // A. Touch wakes → WAKEUP animation first, then HAPPY / long-press → INNOCENT
  if (isTouched) {
      lastInteractionTime = millis();     // touch always resets idle timer

      if (isSleeping) {
          // First touch on a sleeping robot: start WAKEUP
          if (!wakeupFromTouch) {
              eyes.setEmotion(WAKEUP);
              emotionOverrideTimer = millis();
              hasEmotionOverride  = true;
              wakeupFromTouch     = true;
              wakeupTouchTime     = millis();
          }
          // If WAKEUP has played long enough and still touching → go HAPPY
          if (wakeupFromTouch && curEmotion == WAKEUP && (millis() - wakeupTouchTime > 700)) {
              eyes.setEmotion(HAPPY);
              emotionOverrideTimer = millis();
              hasEmotionOverride   = true;
              wakeupFromTouch      = false;
          }
      } else if (curEmotion == WAKEUP && wakeupFromTouch) {
          // Still completing the wake-to-happy transition
          if (millis() - wakeupTouchTime > 700) {
              eyes.setEmotion(HAPPY);
              emotionOverrideTimer = millis();
              hasEmotionOverride   = true;
              wakeupFromTouch      = false;
          }
      } else if (curEmotion == INNOCENT && innocentOverride) {
          // Already in INNOCENT from long-press – keep it alive while touching
          emotionOverrideTimer = millis();
          wasLongPress = true;
      } else if (curEmotion != ANGRY && curEmotion != DIZZY && curEmotion != WAKEUP) {
          // Check for 5s long-press → INNOCENT
          if (!wasLongPress && (millis() - touchHoldStart > 5000)) {
              eyes.setEmotion(INNOCENT);
              emotionOverrideTimer = millis();
              hasEmotionOverride   = true;
              innocentOverride     = true;
              wasLongPress         = true;
          } else if (!innocentOverride) {
              // Short touch while awake → HAPPY (only switch if not already happy)
              if (curEmotion != HAPPY) {
                  eyes.setEmotion(HAPPY);
                  emotionOverrideTimer = millis();
                  hasEmotionOverride   = true;
              } else {
                  emotionOverrideTimer = millis(); // Refresh timer to keep HAPPY alive
              }
          }
      }
  }

  // B. Physical MPU movement wakes from sleep (gentle move only, strong already handled)
  if (physicallyMoved) {
      lastInteractionTime = millis();
      if (!strongPhysical && (curEmotion == SLEEPY || curEmotion == ASLEEP)) {
          eyes.setEmotion(WAKEUP);
          emotionOverrideTimer = millis();
          hasEmotionOverride   = true;
      }
  }

  // C. Camera face: resets idle timer but CANNOT wake from sleep
  if (cameraDetected && !isSleeping) {
      lastInteractionTime = millis();
  }

  wasTouched = isTouched;

  // ── 3. RETURN TO IDLE ────────────────────────────────────────────────────
  curEmotion = eyes.getEmotion();

  // INNOCENT post-release: hold 3 more seconds after finger lifts, then return to idle
  if (innocentOverride && !isTouched) {
      if (millis() - innocentReleaseTime > 3000) {
          eyes.setEmotion(NEUTRAL);
          innocentOverride   = false;
          hasEmotionOverride = false;
      }
  }

  // Generic override timeout (does not fire while INNOCENT is active)
  if (hasEmotionOverride && !isTouched && !innocentOverride && (millis() - emotionOverrideTimer > 3000)) {
      eyes.setEmotion(NEUTRAL);
      hasEmotionOverride = false;
  }

  // ── 4. SLEEP PROGRESSION ─────────────────────────────────────────────────
  if (!hasEmotionOverride && !isTouched && !innocentOverride) {
      unsigned long idleTime = millis() - lastInteractionTime;
      if      (idleTime > 20000 && eyes.getEmotion() != ASLEEP)  eyes.setEmotion(ASLEEP);
      else if (idleTime > 10000 && eyes.getEmotion() != SLEEPY
                                && eyes.getEmotion() != ASLEEP)  eyes.setEmotion(SLEEPY);
  }

  // ── 5. HAPTIC OUTPUT ─────────────────────────────────────────────────────
  curEmotion = eyes.getEmotion();

  if (isTouched && curEmotion == HAPPY) {
      // Purr: smooth sweep 135–255 so motor never stalls
      int purr = 195 + (int)(sin(millis() / 30.0) * 60);
      analogWrite(VIBE_PIN, purr);

  } else if (curEmotion == INNOCENT) {
      // Calm heartbeat: lub-dub pattern repeating every 1200ms
      unsigned long t = millis() % 1200;
      if      (t < 100) analogWrite(VIBE_PIN, 120);
      else if (t < 200) analogWrite(VIBE_PIN, 0);
      else if (t < 300) analogWrite(VIBE_PIN, 90);
      else              analogWrite(VIBE_PIN, 0);

  } else if (wakeupFromTouch) {
      // Double-tap buzz when waking via touch
      unsigned long dt = millis() - wakeupTouchTime;
      if      (dt < 80)  analogWrite(VIBE_PIN, 255);
      else if (dt < 160) analogWrite(VIBE_PIN, 0);
      else if (dt < 250) analogWrite(VIBE_PIN, 210);
      else               analogWrite(VIBE_PIN, 0);

  } else if (curEmotion == ANGRY && hasEmotionOverride) {
      // Angry growl: two hard pulses
      unsigned long dt = millis() - emotionOverrideTimer;
      if      (dt < 130) analogWrite(VIBE_PIN, 240);
      else if (dt < 230) analogWrite(VIBE_PIN, 0);
      else if (dt < 360) analogWrite(VIBE_PIN, 210);
      else               analogWrite(VIBE_PIN, 0);

  } else if (curEmotion == DIZZY && hasEmotionOverride) {
      // Dizzy: erratic irregular buzzing
      unsigned long t = millis();
      int wave = 120 + (int)(abs(sin(t / 70.0)) * 110);
      analogWrite(VIBE_PIN, ((t / 110) % 3 == 0) ? wave : 0);

  } else {
      analogWrite(VIBE_PIN, 0);
  }

  // ── 6. RENDER ────────────────────────────────────────────────────────────
  eyes.update();
  eyes.draw(&sprite);
  sprite.pushSprite(0, 0);
}

// Camera Parsing Function
bool processCameraData() {
  bool detected = false;
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
          
          float x = xStr.toInt() / 100.0; 
          float y = yStr.toInt() / 100.0;
          
          eyes.lookAt(x, y); 
          detected = true;
        }
      }
      packetBuffer = ""; 
    } else {
      packetBuffer += c;
      if (packetBuffer.length() > 50) packetBuffer = ""; 
    }
  }
  
  if (!detected && millis() % 100 == 0) {
      eyes.lookAt(0, 0);
  }
  
  return detected;
}