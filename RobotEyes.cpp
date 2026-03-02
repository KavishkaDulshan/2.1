#include "RobotEyes.h"

void RobotEyes::init() {
  randomSeed(analogRead(1));
}

void RobotEyes::setEmotion(Emotion e) {
  currentEmotion = e;
  blinkState = 0; 
  
  // Reset physics for specific modes
  if (e == SLEEPY) {
    sleepyLidHeight = 0.0;
    sleepDirection = 1;
    easeFactor = 0.05; // Make movement SLOW and SLUGGISH
  } else {
    easeFactor = 0.2; // Reset to snappy speed
  }
}

void RobotEyes::lookAt(float x, float y) {
  targetX = constrain(x, -1.0, 1.0) * 14.0; 
  targetY = constrain(y, -1.0, 1.0) * 10.0; 
}

void RobotEyes::update() {
  
  // --- PHYSICS 1: MOVEMENT ---
  curX += (targetX - curX) * easeFactor;
  curY += (targetY - curY) * easeFactor;

  unsigned long now = millis();

  // --- PHYSICS 2: EMOTION SPECIFIC BEHAVIORS ---

  // A. HAPPY BOUNCE (The "Giggle")
  if (currentEmotion == HAPPY) {
     // Oscillate the eyes up and down using Sine wave
     happyBounceAngle += 0.15; 
     happyBounceY = sin(happyBounceAngle) * 3.0; // Bob up/down by 3 pixels
  } 
  else {
     happyBounceY = 0;
  }

  // B. SLEEPY DROOP (The "Nod")
  if (currentEmotion == SLEEPY) {
    if (now - lastSleepCheck > 60) { // Slower update for sleepy feel
      
      // 1. Move Lids
      sleepyLidHeight += 0.008 * sleepDirection; 

      // 2. Logic: If lids get heavy, force pupil DOWN
      // This overrides the lookAt command to simulate head nodding
      targetY = (sleepyLidHeight * 15.0); 

      // 3. Waking up logic
      if (sleepyLidHeight >= 0.95) { // Fully Closed
         sleepyLidHeight = 0.95;
         if (random(0, 100) > 90) sleepDirection = -1; // Jerk awake
      }
      else if (sleepyLidHeight <= 0.4) { // Eyes half open
         sleepDirection = 1; // Fall back asleep
      }
      lastSleepCheck = now;
    }
    blinkState = sleepyLidHeight;
  }
  
  // C. NORMAL BLINK (Neutral/Angry)
  else if (currentEmotion != HAPPY) {
    if (!isBlinking && (now - lastBlinkTime > blinkInterval)) {
      isBlinking = true;
      blinkInterval = random(2000, 5000);
    }
    
    if (isBlinking) {
      blinkState += 0.25; 
      if (blinkState >= 1.0) {
         blinkState = 1.0;
         isBlinking = false;
         lastBlinkTime = now;
      }
    } else if (blinkState > 0) {
      blinkState -= 0.25; 
      if (blinkState < 0) blinkState = 0;
    }
  }
}

void RobotEyes::draw(LGFX_Sprite *spr) {
  spr->fillScreen(TFT_BLACK);
  int centerX = 64; 
  int centerY = 32; 

  // Apply the Happy Bounce Offset to the Y position
  int drawY = centerY + (int)happyBounceY;

  drawEye(spr, centerX - eyeGap, drawY, -1); 
  drawEye(spr, centerX + eyeGap, drawY, 1);  
}

void RobotEyes::drawEye(LGFX_Sprite *spr, int x, int y, int side) {
  
  int currentH = eyeH;
  int currentW = eyeW;

  // --- 1. SCLERA (White Part) ---
  
  if (currentEmotion == HAPPY) {
      // HAPPY SHAPE: Inverted Crescent (Cheek pushing up)
      // Draw full white eye first
      spr->fillRoundRect(x - eyeW/2, y - eyeH/2, eyeW, eyeH, eyeR, TFT_WHITE);
      
      // Draw BLACK circle at bottom to create the "Cheek"
      // This makes the eye look like an upside down moon ^
      int cheekSize = eyeW + 8;
      spr->fillCircle(x, y + eyeH/2 + 2, cheekSize/2, TFT_BLACK);
      
      // No pupil for happy (cartoon style)
      return; 
  }
  
  // NORMAL/SLEEPY/ANGRY SHAPE
  // Calculate blink height
  currentH = max(2, (int)(eyeH * (1.0 - blinkState)));
  
  // Draw White Eye
  spr->fillRoundRect(x - eyeW/2, y - currentH/2, eyeW, currentH, eyeR, TFT_WHITE);

  // --- 2. PUPIL (Black Part) ---
  if (currentH > 8) {
     int pX = x + curX;
     int pY = y + curY;
     
     // Constraint: Keep pupil inside the white area
     int limitY = (currentH / 2) - pupilR - 2;
     pY = constrain(pY, y - limitY, y + limitY);

     // ANGRY: Pupil shrinks slightly for "intense" look
     int effectivePupilR = (currentEmotion == ANGRY) ? pupilR - 3 : pupilR;

     spr->fillCircle(pX, pY, effectivePupilR, TFT_BLACK);
     
     // CATCHLIGHT (White dot)
     spr->fillCircle(pX + 3, pY - 3, 2, TFT_WHITE);
  }

  // --- 3. EYEBROWS (The Angry Mask) ---
  if (currentEmotion == ANGRY) {
      // Draw a sharp black triangle over the top corner
      // Side -1 (Left): Mask Top-Right
      // Side  1 (Right): Mask Top-Left
      int maskSize = eyeW / 1.5;
      
      if (side == -1) { // Left Eye
         spr->fillTriangle(x + eyeW/2, y - eyeH/2 + 15, // Inner Low 
                           x + eyeW/2, y - eyeH/2 - 5,  // Inner High (above eye)
                           x - 10,     y - eyeH/2 - 5,  // Outer High
                           TFT_BLACK);
      } else { // Right Eye
         spr->fillTriangle(x - eyeW/2, y - eyeH/2 + 15, // Inner Low
                           x - eyeW/2, y - eyeH/2 - 5,  // Inner High
                           x + 10,     y - eyeH/2 - 5,  // Outer High
                           TFT_BLACK);
      }
      
      // Draw the actual Eyebrow Line (White) just above the cut for emphasis?
      // No, let's keep it simple black mask for now.
  }
}