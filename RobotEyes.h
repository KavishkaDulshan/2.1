#ifndef ROBOT_EYES_H
#define ROBOT_EYES_H

#include <LovyanGFX.hpp>

enum Emotion { NEUTRAL, HAPPY, ANGRY, SAD, SLEEPY, INNOCENT };

class RobotEyes {
  private:
    // --- CONFIG ---
    int eyeW = 32;
    int eyeH = 42;
    int eyeR = 12;
    int eyeGap = 28;
    int pupilR = 10;

    // --- PHYSICS STATE ---
    float curX = 0, curY = 0;
    float targetX = 0, targetY = 0;
    float easeFactor = 0.2;

    Emotion currentEmotion = NEUTRAL;

    // Normal Blink (NEUTRAL / ANGRY / SAD / INNOCENT)
    unsigned long lastBlinkTime = 0;
    int blinkInterval = 3000;
    bool isBlinking = false;
    float blinkState = 0.0;

    // ---- SLEEPY ----
    float sleepyLidHeight = 0.0;
    unsigned long lastSleepCheck = 0;
    // Phase-based state machine
    // 0=init  1=drooping  2=closed  3=snap_open  4=hold_open
    int sleepPhase = 0;
    unsigned long sleepPhaseTimer = 0;
    float sleepTrembleAngle = 0.0;
    // Breathing
    float sleepBreathAngle = 0.0;
    float sleepBreathY = 0.0;
    // Micro-drift when drowsy but partially open
    float sleepMicroDriftX = 0.0;
    float sleepMicroTargetX = 0.0;
    unsigned long lastMicroDrift = 0;

    // ---- HAPPY ----
    float happyBounceY = 0;
    float happyBounceAngle = 0;
    float happyShimmerAngle = 0.0;
    float happyBlinkState = 0.0;
    bool happyIsBlinking = false;
    unsigned long lastHappyBlinkTime = 0;
    int happyBlinkInterval = 4000;

    // ---- INNOCENT ----
    float innocentPulseAngle = 0.0;

  public:
    void init();
    void update();
    void draw(LGFX_Sprite *spr);
    void lookAt(float x, float y);
    void setEmotion(Emotion e);
    Emotion getEmotion() { return currentEmotion; } 


  private:
    void drawEye(LGFX_Sprite *spr, int x, int y, int side);
};

#endif
