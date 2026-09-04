/*
  Tabletop Game Timer
  
  Turn-based countdown timer configured with an 8-position rotary switch,
  remote player jacks, main button, passive piezo sounder (B0D7SC3ZFG),
  and a 5-LED display.
*/

// ==========================================
// PIN DEFINITIONS
// ==========================================
const uint8_t rotaryPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t numRotaryPositions = sizeof(rotaryPins) / sizeof(rotaryPins[0]);

const uint8_t jackPins[] = {A3, A4};
const uint8_t numJacks = sizeof(jackPins) / sizeof(jackPins[0]);

const uint8_t mainButtonPin = A2;
const uint8_t buzzerPin = A1; // Passive Piezo Sounder (B0D7SC3ZFG)

// LED Pins: LED1 = D10 (PWM), LED2 = D11 (PWM), LED3 = D12, LED4 = D13, LED5 = A0
const uint8_t ledPins[] = {10, 11, 12, 13, A0};
const uint8_t numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

// ==========================================
// CONFIGURATION & PRESETS
// ==========================================
// Durations for Positions 1 to 8: 1m, 2m, 3m, 5m, 10m, 15m, 15s, 30s
const uint32_t durationPresetsMs[] = {
  60000UL, 120000UL, 180000UL, 300000UL,
  600000UL, 900000UL, 15000UL, 30000UL
};

// Input debouncing & multi-tap detection
const uint32_t debounceDelayMs = 35;
const uint32_t doubleTapWindowMs = 380;

// Alarm sequence: 3 beeps driven at the piezo's resonant frequency (3.1 kHz)
const uint8_t maxAlarmBeeps = 3;
const uint32_t alarmBeepDurationMs = 200;
const uint16_t alarmResonantFreqHz = 3100;

// Turn-start sweep: 2 runs (LED1 -> LED5 -> LED1, 9 steps over ~0.5s)
const uint8_t startSweepPattern[] = {0, 1, 2, 3, 4, 3, 2, 1, 0};
const uint8_t numSweepSteps = sizeof(startSweepPattern);
const uint32_t sweepStepIntervalMs = 55; // 9 * 55ms = 495ms

// Turn-start tone: Warm ~450 Hz "boooop" (1111 µs half-period) over 120 ms
const uint32_t startToneDurationMs = 120;
const uint32_t toneToggleHalfPeriodUs = 1111;

// Sleep animation (center-out sweep + hardware PWM breathing)
const uint32_t sleepOutwardStepMs = 140;
const uint32_t pulseUpdateIntervalMs = 20;

// ==========================================
// STATE MACHINE & RUNTIME VARIABLES
// ==========================================
enum TimerState {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_ALARM,
  STATE_START_ALERT,
  STATE_SLEEP_SWEEP,
  STATE_SLEEPING
};

TimerState currentState = STATE_IDLE;

// Timing counters
uint32_t totalDurationMs = 60000UL;
uint32_t timeRemainingMs = 60000UL;
uint32_t lastTimerUpdateMs = 0;

// Alarm tracking
uint32_t lastAlarmToggleMs = 0;
uint8_t alarmBeepCount = 0;
bool alarmBeepActive = false;

// Turn-start animation tracking
uint32_t startAlertBeginMs = 0;
uint32_t lastSweepStepMs = 0;
uint32_t lastToneToggleUs = 0;
uint8_t currentSweepIndex = 0;
bool tonePinState = false;

// Sleep sequence tracking
uint32_t lastSleepSweepMs = 0;
uint32_t lastPulseUpdateMs = 0;
uint8_t sleepSweepStep = 0;
int16_t pulseBrightness = 0;
int8_t pulseDirection = 4;

// Button debounce tracking
bool lastRawMainBtn = HIGH;
bool stableMainBtn = HIGH;
uint32_t lastMainDebounceMs = 0;

bool lastRawJack[numJacks] = {HIGH, HIGH};
bool stableJack[numJacks] = {HIGH, HIGH};
uint32_t lastJackDebounceMs[numJacks] = {0, 0};

uint8_t clickCount = 0;
uint32_t lastClickTimeMs = 0;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================
void handleInputs();
void updateTimer();
void updateStartAlert();
void updateSleepAnimation();
void updateRunningLeds();
void updateAlarm();
void setAllLeds(uint8_t state);
void resetPwmPins();
void silenceBuzzer();
uint32_t readSelectedDuration();
void triggerTurnStart();
void triggerSleepSequence();

// ==========================================
// SETUP & MAIN LOOP
// ==========================================
void setup() {
  for (uint8_t i = 0; i < numRotaryPositions; i++) {
    pinMode(rotaryPins[i], INPUT_PULLUP);
  }

  for (uint8_t i = 0; i < numJacks; i++) {
    pinMode(jackPins[i], INPUT_PULLUP);
  }

  pinMode(mainButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  silenceBuzzer();

  for (uint8_t i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  totalDurationMs = readSelectedDuration();
  timeRemainingMs = totalDurationMs;
}

void loop() {
  handleInputs();
  updateTimer();
  updateStartAlert();
  updateSleepAnimation();
  updateRunningLeds();
  updateAlarm();
}

// ==========================================
// HELPER UTILITIES
// ==========================================
void setAllLeds(uint8_t state) {
  for (uint8_t i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], state);
  }
}

void resetPwmPins() {
  analogWrite(ledPins[0], 0);
  analogWrite(ledPins[1], 0);
}

void silenceBuzzer() {
  noTone(buzzerPin);
  digitalWrite(buzzerPin, LOW);
  tonePinState = false;
}

uint32_t readSelectedDuration() {
  for (uint8_t i = 0; i < numRotaryPositions; i++) {
    if (digitalRead(rotaryPins[i]) == LOW) {
      return durationPresetsMs[i];
    }
  }
  return durationPresetsMs[0];
}

// ==========================================
// STATE TRIGGERS
// ==========================================
void triggerTurnStart() {
  startAlertBeginMs = millis();
  lastSweepStepMs = startAlertBeginMs;
  lastToneToggleUs = micros();
  currentSweepIndex = 0;

  resetPwmPins();
  setAllLeds(LOW);
  digitalWrite(ledPins[startSweepPattern[0]], HIGH);

  currentState = STATE_START_ALERT;
}

void triggerSleepSequence() {
  silenceBuzzer();
  resetPwmPins();
  setAllLeds(LOW);

  sleepSweepStep = 0;
  lastSleepSweepMs = millis();
  currentState = STATE_SLEEP_SWEEP;
}

// ==========================================
// INPUT HANDLING & DEBOUNCE
// ==========================================
void handleInputs() {
  const uint32_t now = millis();
  bool pressDetected = false;

  // 1. Debounce Main Pushbutton
  bool rawMain = digitalRead(mainButtonPin);
  if (rawMain != lastRawMainBtn) {
    lastMainDebounceMs = now;
  }
  lastRawMainBtn = rawMain;

  if ((now - lastMainDebounceMs) > debounceDelayMs) {
    if (rawMain != stableMainBtn) {
      stableMainBtn = rawMain;
      if (stableMainBtn == LOW) {
        pressDetected = true;
      }
    }
  }

  // 2. Debounce External Player Jacks
  for (uint8_t i = 0; i < numJacks; i++) {
    bool rawJack = digitalRead(jackPins[i]);
    if (rawJack != lastRawJack[i]) {
      lastJackDebounceMs[i] = now;
    }
    lastRawJack[i] = rawJack;

    if ((now - lastJackDebounceMs[i]) > debounceDelayMs) {
      if (rawJack != stableJack[i]) {
        stableJack[i] = rawJack;
        if (stableJack[i] == LOW) {
          pressDetected = true;
        }
      }
    }
  }

  // 3. Multi-tap Resolution
  if (pressDetected) {
    clickCount++;
    lastClickTimeMs = now;

    if (clickCount >= 2) {
      clickCount = 0;
      triggerSleepSequence();
    }
  }

  if (clickCount == 1 && (now - lastClickTimeMs > doubleTapWindowMs)) {
    clickCount = 0;
    triggerTurnStart();
  }
}

// ==========================================
// ALERT, SLEEP, AND ALARM LOGIC
// ==========================================
void updateStartAlert() {
  if (currentState != STATE_START_ALERT) {
    return;
  }

  const uint32_t nowMs = millis();
  const uint32_t toneElapsedMs = nowMs - startAlertBeginMs;

  // Clean ~450 Hz "boooop" on passive piezo with soft envelope
  if (toneElapsedMs < startToneDurationMs) {
    const uint32_t nowUs = micros();
    if (nowUs - lastToneToggleUs >= toneToggleHalfPeriodUs) {
      lastToneToggleUs = nowUs;
      tonePinState = !tonePinState;

      bool allowPulse = true;
      if (toneElapsedMs < 20) {
        // Soft attack ramp
        allowPulse = (toneElapsedMs % 8 < 4);
      } else if (toneElapsedMs > (startToneDurationMs - 25)) {
        // Soft decay tail
        allowPulse = (toneElapsedMs % 8 < 3);
      }

      digitalWrite(buzzerPin, (tonePinState && allowPulse) ? HIGH : LOW);
    }
  } else {
    silenceBuzzer();
  }

  // 2-run sweep animation (LED1 -> LED5 -> LED1)
  if (nowMs - lastSweepStepMs >= sweepStepIntervalMs) {
    lastSweepStepMs = nowMs;
    currentSweepIndex++;

    if (currentSweepIndex >= numSweepSteps) {
      silenceBuzzer();
      setAllLeds(LOW);

      totalDurationMs = readSelectedDuration();
      timeRemainingMs = totalDurationMs;
      lastTimerUpdateMs = millis();
      currentState = STATE_RUNNING;
      return;
    }

    const uint8_t activeLed = startSweepPattern[currentSweepIndex];
    for (uint8_t i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], (i == activeLed) ? HIGH : LOW);
    }
  }
}

void updateSleepAnimation() {
  const uint32_t nowMs = millis();

  // Phase 1: Center-out sweep
  if (currentState == STATE_SLEEP_SWEEP) {
    if (nowMs - lastSleepSweepMs >= sleepOutwardStepMs) {
      lastSleepSweepMs = nowMs;
      sleepSweepStep++;

      setAllLeds(LOW);

      if (sleepSweepStep == 1) {
        digitalWrite(ledPins[2], HIGH); // Center LED3
      } else if (sleepSweepStep == 2) {
        digitalWrite(ledPins[1], HIGH); // LED2 & LED4
        digitalWrite(ledPins[3], HIGH);
      } else if (sleepSweepStep == 3) {
        digitalWrite(ledPins[0], HIGH); // LED1 & LED5
        digitalWrite(ledPins[4], HIGH);
      } else {
        pulseBrightness = 0;
        pulseDirection = 4;
        currentState = STATE_SLEEPING;
      }
    }
    return;
  }

  // Phase 2: Hardware PWM breathing on LED1 (D10) and LED2 (D11) in antiphase
  if (currentState == STATE_SLEEPING) {
    if (nowMs - lastPulseUpdateMs >= pulseUpdateIntervalMs) {
      lastPulseUpdateMs = nowMs;

      pulseBrightness += pulseDirection;
      if (pulseBrightness >= 255) {
        pulseBrightness = 255;
        pulseDirection = -pulseDirection;
      } else if (pulseBrightness <= 0) {
        pulseBrightness = 0;
        pulseDirection = -pulseDirection;
      }

      analogWrite(ledPins[0], pulseBrightness);
      analogWrite(ledPins[1], 255 - pulseBrightness);
    }
  }
}

void updateTimer() {
  if (currentState != STATE_RUNNING) {
    return;
  }

  const uint32_t nowMs = millis();
  const uint32_t elapsedMs = nowMs - lastTimerUpdateMs;
  lastTimerUpdateMs = nowMs;

  if (elapsedMs >= timeRemainingMs) {
    timeRemainingMs = 0;
    currentState = STATE_ALARM;
    alarmBeepCount = 0;
    alarmBeepActive = false;
    lastAlarmToggleMs = nowMs;
  } else {
    timeRemainingMs -= elapsedMs;
  }
}

void updateRunningLeds() {
  if (currentState != STATE_RUNNING) {
    if (currentState == STATE_IDLE || currentState == STATE_ALARM) {
      setAllLeds(LOW);
    }
    return;
  }

  const uint32_t elapsedMs = totalDurationMs - timeRemainingMs;
  const uint32_t stageDurationMs = totalDurationMs / numLeds;
  uint8_t activeStage = elapsedMs / stageDurationMs;

  if (activeStage >= numLeds) {
    activeStage = numLeds - 1;
  }

  setAllLeds(LOW);

  // During the final half of the last segment (last 10%), blink LED5
  const uint32_t finalThresholdMs = totalDurationMs - (stageDurationMs / 2);
  if (elapsedMs >= finalThresholdMs) {
    bool blink = (millis() / 250) % 2 == 0;
    digitalWrite(ledPins[4], blink ? HIGH : LOW);
  } else {
    digitalWrite(ledPins[activeStage], HIGH);
  }
}

void updateAlarm() {
  if (currentState != STATE_ALARM) {
    return;
  }

  if (alarmBeepCount >= maxAlarmBeeps) {
    silenceBuzzer();
    return;
  }

  const uint32_t nowMs = millis();
  if (nowMs - lastAlarmToggleMs >= alarmBeepDurationMs) {
    lastAlarmToggleMs = nowMs;
    alarmBeepActive = !alarmBeepActive;

    if (alarmBeepActive) {
      // Passive piezo sounder driven at resonant peak for maximum volume
      tone(buzzerPin, alarmResonantFreqHz);
    } else {
      noTone(buzzerPin);
      digitalWrite(buzzerPin, LOW);
      alarmBeepCount++;
    }
  }
}