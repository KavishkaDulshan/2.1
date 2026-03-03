#include "RobotEyes.h"

void RobotEyes::init()
{
  randomSeed(analogRead(1));
}

void RobotEyes::setEmotion(Emotion e)
{
  currentEmotion = e;
  blinkState = 0;
  isBlinking = false;

  if (e == SLEEPY)
  {
    sleepyLidHeight = 0.05f;
    sleepPhase = 0;
    sleepPhaseTimer = millis();
    sleepTrembleAngle = 0;
    sleepMicroDriftX = 0;
    sleepMicroTargetX = 0;
    lastMicroDrift = 0;
    targetY = 0;
    curY = 0;
    easeFactor = 0.05f;
  }
  else if (e == ASLEEP)
  {
    // Deep Sleep Mode - uses SLEEPY drawEye with lid at 0.92
    sleepBreathAngle = 0;
    sleepyLidHeight = 0.92f;
    sleepPhase = 0;
    sleepPhaseTimer = millis();
    sleepTrembleAngle = 0;
    sleepMicroDriftX = 0;
    sleepMicroTargetX = 0;
    blinkState = 0; // SLEEPY drawEye handles rendering
    targetY = 20.0f;
    curY = 20.0f;
    easeFactor = 0.04f;
  }
  else if (e == HAPPY)
  {
    happyBounceAngle = 0;
    happyShimmerAngle = 0;
    easeFactor = 0.2f;
  }
  else if (e == SAD)
  {
    targetX = 0;
    targetY = 8.0f;
    easeFactor = 0.08f;
  }
  else if (e == INNOCENT)
  {
    targetX = 0;
    targetY = -5.0f;
    easeFactor = 0.15f;
  }
  else if (e == DIZZY)
  {
    dizzyAngle = 0;
    easeFactor = 0.3f; // Fast, erratic
  }
  else if (e == WAKEUP)
  {
    targetX = 0;
    targetY = 0;
    blinkState = 1.0f; // Start closed, will snap open in update
    easeFactor = 0.3f;
  }
  else
  {
    targetX = 0;
    targetY = 0;
    easeFactor = 0.2f;
  }
}

void RobotEyes::lookAt(float x, float y)
{
  // Only controls PUPIL
  targetX = constrain(x, -1.0, 1.0) * 14.0;
  targetY = constrain(y, -1.0, 1.0) * 10.0;
}

void RobotEyes::setEyeOffset(float x, float y)
{
  // Controls WHOLE EYE (Sclera) bounds
  targetEyeOffsetX = constrain(x, -15.0, 15.0);
  targetEyeOffsetY = constrain(y, -15.0, 15.0);
}

void RobotEyes::update()
{
  // Smooth movement for Pupil
  curX += (targetX - curX) * easeFactor;
  curY += (targetY - curY) * easeFactor;

  // Smooth movement for Whole Eye Offset
  eyeOffsetX += (targetEyeOffsetX - eyeOffsetX) * 0.15f;
  eyeOffsetY += (targetEyeOffsetY - eyeOffsetY) * 0.15f;

  unsigned long now = millis();

  // --- NEW: DIZZY ANIMATION ---
  if (currentEmotion == DIZZY)
  {
    dizzyAngle += 0.4f;
    // Spin pupils in circles
    targetX = sin(dizzyAngle) * 10.0f;
    targetY = cos(dizzyAngle) * 10.0f;
    // Wobble blink slightly
    blinkState = 0.2f + (sin(dizzyAngle * 0.5f) * 0.1f);
    return;
  }

  // --- NEW: WAKEUP ANIMATION ---
  if (currentEmotion == WAKEUP)
  {
    if (blinkState > 0.0f)
    {
      blinkState -= 0.15f; // Snap open rapidly
      if (blinkState <= 0)
        blinkState = 0;
    }
    return; // Hold wide open
  }

  // --- ASLEEP ANIMATION (Deep Sleep with REM phases) ---
  if (currentEmotion == ASLEEP)
  {
    sleepBreathAngle += 0.010f; // Very slow, heavy breathing
    sleepBreathY = sin(sleepBreathAngle) * 2.5f;
    sleepTrembleAngle += 0.08f;
    if (now - lastSleepCheck > 50)
    {
      switch (sleepPhase)
      {
      case 0: // Deep sleep: lid barely stirs
        sleepyLidHeight = 0.92f + sin(sleepTrembleAngle) * 0.012f;
        if (now - sleepPhaseTimer > (unsigned long)random(3000, 6000))
        {
          sleepPhase = 1;
          sleepPhaseTimer = now;
        }
        break;
      case 1: // REM: lid slowly drifts open (dreaming)
        sleepyLidHeight -= 0.008f;
        if (sleepyLidHeight <= 0.60f)
        {
          sleepyLidHeight = 0.60f;
          sleepPhase = 2;
          sleepPhaseTimer = now;
        }
        break;
      case 2: // REM: flutter briefly
        sleepyLidHeight = 0.60f + sin(sleepTrembleAngle * 1.5f) * 0.04f;
        if (now - sleepPhaseTimer > (unsigned long)random(400, 900))
        {
          sleepPhase = 3;
          sleepPhaseTimer = now;
        }
        break;
      case 3: // REM: close back to deep sleep
        sleepyLidHeight += 0.010f;
        if (sleepyLidHeight >= 0.92f)
        {
          sleepyLidHeight = 0.92f;
          sleepPhase = 0;
          sleepPhaseTimer = now;
        }
        break;
      }
      lastSleepCheck = now;
    }
    blinkState = 0; // SLEEPY drawEye handles rendering via sleepyLidHeight
    return;
  }

  // --- HAPPY ---
  if (currentEmotion == HAPPY)
  {
    happyBounceAngle += 0.15f;
    happyBounceY = sin(happyBounceAngle) * 3.0f;
    happyShimmerAngle += 0.10f;
    if (!happyIsBlinking && (now - lastHappyBlinkTime > (unsigned long)happyBlinkInterval))
    {
      happyIsBlinking = true;
      happyBlinkInterval = random(2500, 5000);
    }
    if (happyIsBlinking)
    {
      happyBlinkState += 0.20f;
      if (happyBlinkState >= 1.0f)
      {
        happyBlinkState = 1.0f;
        happyIsBlinking = false;
        lastHappyBlinkTime = now;
      }
    }
    else if (happyBlinkState > 0)
    {
      happyBlinkState -= 0.20f;
      if (happyBlinkState < 0)
        happyBlinkState = 0;
    }
    return;
  }

  // --- SLEEPY (Your existing awesome state machine) ---
  if (currentEmotion == SLEEPY)
  {
    sleepBreathAngle += 0.018f;
    sleepBreathY = sin(sleepBreathAngle) * 1.2f;
    sleepTrembleAngle += 0.30f;
    if (now - lastSleepCheck > 50)
    {
      switch (sleepPhase)
      {
      case 0:
        sleepyLidHeight = 0.05f;
        targetY = 0;
        sleepPhase = 1;
        sleepPhaseTimer = now;
        break;
      case 1:
      {
        sleepyLidHeight += 0.010f;
        targetY = sleepyLidHeight * 22.0f;
        // Lazy micro-drift: pupils slowly wander while drooping
        if (now - lastMicroDrift > 300)
        {
          sleepMicroTargetX = (float)random(-5, 6);
          lastMicroDrift = now;
        }
        sleepMicroDriftX += (sleepMicroTargetX - sleepMicroDriftX) * 0.04f;
        if (sleepyLidHeight >= 0.85f)
        {
          sleepyLidHeight = 0.85f;
          sleepPhase = 2;
          sleepPhaseTimer = now;
        }
        break;
      }
      case 2:
        targetY = 22.0f;
        sleepyLidHeight = 0.85f + sin(sleepTrembleAngle * 0.4f) * 0.02f;
        if (now - sleepPhaseTimer > (unsigned long)random(900, 2200))
        {
          sleepPhase = 3;
          sleepPhaseTimer = now;
          targetY = 3.0f;
        }
        break;
      case 3:
        sleepyLidHeight -= 0.050f;
        if (sleepyLidHeight <= 0.20f)
        {
          sleepyLidHeight = 0.20f;
          sleepPhase = 4;
          sleepPhaseTimer = now;
          targetY = 7.0f;
        }
        break;
      case 4:
        sleepyLidHeight = 0.20f + fabs(sin(sleepTrembleAngle * 0.6f)) * 0.22f;
        targetY = 7.0f + sin(sleepTrembleAngle * 0.3f) * 2.0f;
        if (now - sleepPhaseTimer > (unsigned long)random(450, 950))
        {
          sleepPhase = 1;
          sleepPhaseTimer = now;
        }
        break;
      }
      lastSleepCheck = now;
    }
    blinkState = 0;
    return;
  }

  // --- INNOCENT ---
  if (currentEmotion == INNOCENT)
  {
    innocentPulseAngle += 0.04f;
    targetY = -5.0f;
  }

  // Normal blink
  float blinkSpeed = (currentEmotion == INNOCENT) ? 0.10f : 0.25f;
  if (!isBlinking && (now - lastBlinkTime > (unsigned long)blinkInterval))
  {
    isBlinking = true;
    blinkInterval = random(2000, 6000);
  }
  if (isBlinking)
  {
    blinkState += blinkSpeed;
    if (blinkState >= 1.0f)
    {
      blinkState = 1.0f;
      isBlinking = false;
      lastBlinkTime = now;
    }
  }
  else if (blinkState > 0)
  {
    blinkState -= blinkSpeed;
    if (blinkState < 0)
      blinkState = 0;
  }
}

void RobotEyes::draw(LGFX_Sprite *spr)
{
  spr->fillScreen(TFT_BLACK);

  // APPLY MPU6050 EYE OFFSET HERE
  int centerX = 64 + (int)eyeOffsetX;
  int centerY = 32 + (int)eyeOffsetY;
  int drawY = centerY;

  if (currentEmotion == HAPPY)
    drawY = centerY + (int)happyBounceY;
  if (currentEmotion == SLEEPY || currentEmotion == ASLEEP)
    drawY = centerY + (int)sleepBreathY;

  drawEye(spr, centerX - eyeGap, drawY, -1);
  drawEye(spr, centerX + eyeGap, drawY, 1);
}

void RobotEyes::drawEye(LGFX_Sprite *spr, int x, int y, int side)
{
  // HAPPY
  if (currentEmotion == HAPPY)
  {
    int happyH = (happyBlinkState > 0) ? max(3, (int)(eyeH * (1.0f - happyBlinkState * 0.85f))) : eyeH;
    spr->fillRoundRect(x - eyeW / 2, y - happyH / 2, eyeW, happyH, eyeR, TFT_WHITE);
    spr->fillCircle(x, y + happyH / 2 + 2, (eyeW + 8 + (happyBounceY > 1.5f ? 2 : 0)) / 2, TFT_BLACK);
    return;
  }

  // SLEEPY / ASLEEP (shared curved lid rendering)
  if (currentEmotion == SLEEPY || currentEmotion == ASLEEP)
  {
    int eyeTop = y - eyeH / 2;
    spr->fillRoundRect(x - eyeW / 2, eyeTop, eyeW, eyeH, eyeR, TFT_WHITE);
    int lidR = 44;
    int lidCY = eyeTop - lidR + (int)(eyeH * sleepyLidHeight);
    int lidCX = x + (-side) * (int)(sleepyLidHeight * 5);
    spr->fillCircle(lidCX, lidCY, lidR, TFT_BLACK);
    int lidLineY = eyeTop + (int)(eyeH * sleepyLidHeight);
    int visH = y + eyeH / 2 - lidLineY;
    // Only draw pupil for SLEEPY, not ASLEEP (eyes closed during sleep)
    if (currentEmotion == SLEEPY && visH > 8)
    {
      int pX = x + (int)curX + (int)sleepMicroDriftX;
      int pY = constrain(y + (int)curY, lidLineY + pupilR + 2, y + eyeH / 2 - pupilR - 2);
      int effR = (visH < 24) ? max(4, pupilR - (24 - visH) / 3) : pupilR;
      spr->fillCircle(pX, pY, effR, TFT_BLACK);
      if (visH > 18)
        spr->fillCircle(pX + 3, pY - 3, 2, TFT_WHITE);
    }
    return;
  }

  // STANDARD RENDERING (Neutral, Angry, Sad, Dizzy, Wakeup)
  int h = max(2, (int)(eyeH * (1.0f - blinkState)));
  spr->fillRoundRect(x - eyeW / 2, y - h / 2, eyeW, h, eyeR, TFT_WHITE);

  if (h > 8)
  {
    int pX = x + curX;
    int pY = constrain(y + (int)curY, y - h / 2 + pupilR + 2, y + h / 2 - pupilR - 2);

    // Dizzy pupil shrinks and expands slightly
    int effR = pupilR;
    if (currentEmotion == ANGRY)
      effR = pupilR - 3;
    if (currentEmotion == DIZZY)
      effR = pupilR - 2 + (int)(sin(dizzyAngle) * 2);

    spr->fillCircle(pX, pY, effR, TFT_BLACK);
    spr->fillCircle(pX + 3, pY - 3, 2, TFT_WHITE);
  }

  if (currentEmotion == ANGRY)
  {
    if (side == -1)
      spr->fillTriangle(x + eyeW / 2, y - eyeH / 2 + 15, x + eyeW / 2, y - eyeH / 2 - 5, x - 10, y - eyeH / 2 - 5, TFT_BLACK);
    else
      spr->fillTriangle(x - eyeW / 2, y - eyeH / 2 + 15, x - eyeW / 2, y - eyeH / 2 - 5, x + 10, y - eyeH / 2 - 5, TFT_BLACK);
  }
}