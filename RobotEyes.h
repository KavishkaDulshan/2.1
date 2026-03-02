#ifndef ROBOT_EYES_H
#define ROBOT_EYES_H

#include <LovyanGFX.hpp>

enum Emotion { NEUTRAL, HAPPY, ANGRY, SAD, SLEEPY };

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
    float easeFactor = 0.2; // Default speed
    
    Emotion currentEmotion = NEUTRAL;
    
    // Blink & Sleep
    unsigned long lastBlinkTime = 0;
    int blinkInterval = 3000;
    bool isBlinking = false;
    float blinkState = 0.0; 

    // Sleepy Logic
    float sleepyLidHeight = 0.0;
    unsigned long lastSleepCheck = 0;
    int sleepDirection = 1;

    // Happy Logic (Bouncing)
    float happyBounceY = 0;
    float happyBounceAngle = 0;

  public:
    void init();
    void update();
    void draw(LGFX_Sprite *spr);
    void lookAt(float x, float y);
    void setEmotion(Emotion e);
    
  private:
    void drawEye(LGFX_Sprite *spr, int x, int y, int side);
};

#endif