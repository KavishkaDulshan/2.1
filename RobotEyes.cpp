#include "RobotEyes.h"

void RobotEyes::init() {
  randomSeed(analogRead(1));
}

void RobotEyes::setEmotion(Emotion e) {
  currentEmotion = e;
  blinkState = 0;
  isBlinking = false;

  if (e == SLEEPY) {
    sleepyLidHeight = 0.05f;
    sleepPhase = 0;
    sleepPhaseTimer = millis();
    sleepTrembleAngle = 0.0f;
    sleepBreathAngle = 0.0f;
    sleepMicroDriftX = 0.0f;
    sleepMicroTargetX = 0.0f;
    targetY = 0;
    easeFactor = 0.05f;
  } else if (e == HAPPY) {
    happyBounceAngle = 0.0f;
    happyShimmerAngle = 0.0f;
    happyBlinkState = 0.0f;
    happyIsBlinking = false;
    lastHappyBlinkTime = 0;
    easeFactor = 0.2f;
  } else if (e == SAD) {
    targetX = 0;
    targetY = 8.0f;
    easeFactor = 0.08f;
    blinkInterval = random(4000, 7000);
  } else if (e == INNOCENT) {
    targetX = 0;
    targetY = -5.0f;
    innocentPulseAngle = 0.0f;
    easeFactor = 0.15f;
    blinkInterval = random(3500, 6000);
  } else {
    targetX = 0;
    targetY = 0;
    easeFactor = 0.2f;
  }
}

void RobotEyes::lookAt(float x, float y) {
  targetX = constrain(x, -1.0, 1.0) * 14.0;
  targetY = constrain(y, -1.0, 1.0) * 10.0;
}

void RobotEyes::update() {
  curX += (targetX - curX) * easeFactor;
  curY += (targetY - curY) * easeFactor;
  unsigned long now = millis();

  // HAPPY
  if (currentEmotion == HAPPY) {
    happyBounceAngle += 0.15f;
    happyBounceY = sin(happyBounceAngle) * 3.0f;
    happyShimmerAngle += 0.10f;
    if (!happyIsBlinking && (now - lastHappyBlinkTime > (unsigned long)happyBlinkInterval)) {
      happyIsBlinking = true;
      happyBlinkInterval = random(2500, 5000);
    }
    if (happyIsBlinking) {
      happyBlinkState += 0.20f;
      if (happyBlinkState >= 1.0f) { happyBlinkState = 1.0f; happyIsBlinking = false; lastHappyBlinkTime = now; }
    } else if (happyBlinkState > 0) {
      happyBlinkState -= 0.20f;
      if (happyBlinkState < 0) happyBlinkState = 0;
    }
    return;
  }

  // SLEEPY state machine: 0=init 1=drooping 2=closed 3=snap_open 4=hold_open
  if (currentEmotion == SLEEPY) {
    sleepBreathAngle += 0.018f;
    sleepBreathY = sin(sleepBreathAngle) * 1.2f;
    sleepTrembleAngle += 0.30f;
    if (now - lastSleepCheck > 50) {
      switch (sleepPhase) {
        case 0:
          sleepyLidHeight = 0.05f; targetY = 0;
          sleepPhase = 1; sleepPhaseTimer = now; break;
        case 1: {
          sleepyLidHeight += 0.010f;  // faster droop so full cycle fits in demo
          targetY = sleepyLidHeight * 22.0f;
          if (sleepyLidHeight < 0.45f && (now - lastMicroDrift > 2000)) {
            sleepMicroTargetX = random(-5, 5); lastMicroDrift = now;
          }
          float fade = constrain(1.0f - (sleepyLidHeight / 0.45f), 0.0f, 1.0f);
          sleepMicroDriftX += (sleepMicroTargetX * fade - sleepMicroDriftX) * 0.025f;
          if (sleepyLidHeight >= 0.85f) {
            sleepyLidHeight = 0.85f; sleepPhase = 2; sleepPhaseTimer = now;
          }
          break;
        }
        case 2:
          targetY = 22.0f;
          sleepyLidHeight = 0.85f + sin(sleepTrembleAngle * 0.4f) * 0.02f;
          sleepMicroDriftX *= 0.92f;
          if (now - sleepPhaseTimer > (unsigned long)random(900, 2200)) {
            sleepPhase = 3; sleepPhaseTimer = now; targetY = 3.0f;
          }
          break;
        case 3:
          sleepyLidHeight -= 0.050f;
          if (sleepyLidHeight <= 0.12f) {
            sleepyLidHeight = 0.12f; sleepPhase = 4;
            sleepPhaseTimer = now; targetY = 7.0f;
          }
          break;
        case 4:
          sleepyLidHeight = 0.12f + fabs(sin(sleepTrembleAngle * 0.6f)) * 0.13f;
          targetY = 7.0f + sin(sleepTrembleAngle * 0.3f) * 2.0f;
          if (now - sleepPhaseTimer > (unsigned long)random(450, 950)) {
            sleepPhase = 1; sleepPhaseTimer = now;
            if (sleepyLidHeight < 0.15f) sleepyLidHeight = 0.15f;
          }
          break;
      }
      lastSleepCheck = now;
    }
    blinkState = 0;
    return;
  }

  // INNOCENT
  if (currentEmotion == INNOCENT) {
    innocentPulseAngle += 0.04f;
    targetY = -5.0f;
    targetX += (0 - targetX) * 0.05f;
  }

  // Normal blink: NEUTRAL / ANGRY / SAD / INNOCENT
  float blinkSpeed = (currentEmotion == INNOCENT) ? 0.10f : 0.25f;
  int minI = (currentEmotion == SAD || currentEmotion == INNOCENT) ? 3500 : 2000;
  int maxI = (currentEmotion == SAD) ? 7000 : (currentEmotion == INNOCENT) ? 6500 : 5000;
  if (!isBlinking && (now - lastBlinkTime > (unsigned long)blinkInterval)) {
    isBlinking = true; blinkInterval = random(minI, maxI);
  }
  if (isBlinking) {
    blinkState += blinkSpeed;
    if (blinkState >= 1.0f) { blinkState = 1.0f; isBlinking = false; lastBlinkTime = now; }
  } else if (blinkState > 0) {
    blinkState -= blinkSpeed;
    if (blinkState < 0) blinkState = 0;
  }
}

void RobotEyes::draw(LGFX_Sprite *spr) {
  spr->fillScreen(TFT_BLACK);
  int centerX = 64, centerY = 32, drawY = centerY;
  if (currentEmotion == HAPPY)  drawY = centerY + (int)happyBounceY;
  if (currentEmotion == SLEEPY) drawY = centerY + (int)sleepBreathY;
  drawEye(spr, centerX - eyeGap, drawY, -1);
  drawEye(spr, centerX + eyeGap, drawY,  1);
}

void RobotEyes::drawEye(LGFX_Sprite *spr, int x, int y, int side) {

  // ---- HAPPY: crescent + shimmer + blink ----
  if (currentEmotion == HAPPY) {
    int happyH = (happyBlinkState > 0)
      ? max(3, (int)(eyeH * (1.0f - happyBlinkState * 0.85f))) : eyeH;
    spr->fillRoundRect(x - eyeW/2, y - happyH/2, eyeW, happyH, eyeR, TFT_WHITE);
    spr->fillCircle(x, y + happyH/2 + 2, (eyeW + 8 + (happyBounceY > 1.5f ? 2 : 0)) / 2, TFT_BLACK);
    if (happyBlinkState < 0.4f) {
      int d1y = y - happyH / 4;
      if (d1y < y) spr->fillCircle(x + 6, d1y, 2, TFT_WHITE);
      int d2x = constrain(x + (int)(sin(happyShimmerAngle) * 6.0f) - 2, x - eyeW/2 + 3, x + eyeW/2 - 3);
      int d2y = y - happyH / 4 + (int)(cos(happyShimmerAngle) * 2.5f);
      if (d2y < y && d2y > y - happyH/2 + 2) spr->fillCircle(d2x, d2y, 1, TFT_WHITE);
    }
    return;
  }

  // ---- SLEEPY: smooth curved eyelid via large-circle arc from above ----
  // A large black circle centered above droops its bottom arc into the eye,
  // forming a soft organic eyelid curve � nothing like ANGRY sharp triangles.
  if (currentEmotion == SLEEPY) {
    int eyeTop = y - eyeH / 2;

    // 1. Full white eye base
    spr->fillRoundRect(x - eyeW/2, eyeTop, eyeW, eyeH, eyeR, TFT_WHITE);

    // 2. Curved eyelid � large circle whose bottom arc IS the lid line
    int lidR = 44;   // large radius = gentle, natural eyelid curvature

    // Lid center Y: as sleepyLidHeight goes 0->1, circle sinks into the eye
    int lidCY = eyeTop - lidR + (int)(eyeH * sleepyLidHeight);

    // Subtle asymmetry: inner/nasal corner droops slightly more
    // side=-1 (left): inner=right, shift circle right; side=1 (right): shift left
    int lidCX = x + (-side) * (int)(sleepyLidHeight * 5);

    spr->fillCircle(lidCX, lidCY, lidR, TFT_BLACK);

    // 3. Pupil in the visible slit below the lid
    // Lid bottom at eye center = eyeTop + eyeH*sleepyLidHeight
    int lidLineY = eyeTop + (int)(eyeH * sleepyLidHeight);
    int visH    = y + eyeH/2 - lidLineY;

    if (visH > 8) {
      int pX = x + (int)curX + (int)sleepMicroDriftX;
      int visTop = lidLineY + pupilR + 2;
      int visBot = y + eyeH/2 - pupilR - 2;
      int pY = constrain(y + (int)curY, visTop, visBot);
      int effR = (visH < 24) ? max(4, pupilR - (24 - visH) / 3) : pupilR;
      spr->fillCircle(pX, pY, effR, TFT_BLACK);
      if (visH > 18) spr->fillCircle(pX + 3, pY - 2, 2, TFT_WHITE);
    }
    return;
  }

  // ---- SAD: drooping outer corners, down-outward gaze, dim catchlight ----
  if (currentEmotion == SAD) {
    int h = max(2, (int)(eyeH * (1.0f - blinkState)));
    spr->fillRoundRect(x - eyeW/2, y - h/2, eyeW, h, eyeR, TFT_WHITE);
    if (h > 8) {
      int pX = x + (int)curX + side * 4;
      int pY = constrain(y + (int)curY, y - h/2 + pupilR + 2, y + h/2 - pupilR - 2);
      spr->fillCircle(pX, pY, pupilR, TFT_BLACK);
      spr->fillCircle(pX + 2, pY - 2, 1, TFT_WHITE);
    }
    // Outer corner droop (opposite of ANGRY)
    if (side == -1) {
      spr->fillTriangle(x - eyeW/2, y - eyeH/2 + 17, x - eyeW/2, y - eyeH/2 - 5, x + 9, y - eyeH/2 - 5, TFT_BLACK);
    } else {
      spr->fillTriangle(x + eyeW/2, y - eyeH/2 + 17, x + eyeW/2, y - eyeH/2 - 5, x - 9, y - eyeH/2 - 5, TFT_BLACK);
    }
    return;
  }

  // ---- INNOCENT: big pupils, upward gaze, large sparkle catchlights ----
  if (currentEmotion == INNOCENT) {
    int h = max(2, (int)(eyeH * (1.0f - blinkState)));
    // Clamp corner radius so fillRoundRect never gets radius > h/2 (avoids glitch)
    int rr = min(eyeR, max(1, h / 2 - 1));
    spr->fillRoundRect(x - eyeW/2, y - h/2, eyeW, h, rr, TFT_WHITE);
    // Shrink pupil to fit inside the current blink height
    int bigR = pupilR + 3 + (int)(sin(innocentPulseAngle) * 1.0f);
    bigR = min(bigR, h / 2 - 2);  // clamp: pupil must fit inside eye height
    if (bigR >= 2) {
      int pX = constrain(x + (int)curX, x - eyeW/2 + bigR + 1, x + eyeW/2 - bigR - 1);
      int pY = constrain(y + (int)curY, y - h/2 + bigR + 1, y + h/2 - bigR - 1);
      spr->fillCircle(pX, pY, bigR, TFT_BLACK);
      // Only draw catchlights when eye is wide enough to contain them cleanly
      if (h > bigR * 2 + 6) {
        spr->fillCircle(pX + 4, pY - 4, 4, TFT_WHITE);
        spr->fillCircle(pX - 3, pY + 3, 2, TFT_WHITE);
      }
    }
    return;
  }

  // ---- NEUTRAL / ANGRY: normal blink + pupil + brow mask ----
  int h = max(2, (int)(eyeH * (1.0f - blinkState)));
  spr->fillRoundRect(x - eyeW/2, y - h/2, eyeW, h, eyeR, TFT_WHITE);
  if (h > 8) {
    int pX = x + curX;
    int pY = constrain(y + (int)curY, y - h/2 + pupilR + 2, y + h/2 - pupilR - 2);
    int effR = (currentEmotion == ANGRY) ? pupilR - 3 : pupilR;
    spr->fillCircle(pX, pY, effR, TFT_BLACK);
    spr->fillCircle(pX + 3, pY - 3, 2, TFT_WHITE);
  }
  if (currentEmotion == ANGRY) {
    if (side == -1)
      spr->fillTriangle(x + eyeW/2, y - eyeH/2 + 15, x + eyeW/2, y - eyeH/2 - 5, x - 10, y - eyeH/2 - 5, TFT_BLACK);
    else
      spr->fillTriangle(x - eyeW/2, y - eyeH/2 + 15, x - eyeW/2, y - eyeH/2 - 5, x + 10, y - eyeH/2 - 5, TFT_BLACK);
  }
}
