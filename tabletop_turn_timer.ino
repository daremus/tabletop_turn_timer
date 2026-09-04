// ==========================================
// PIN DEFINITIONS
// ==========================================

// Rotary Switch Positions 1 through 8
const int ROTARY_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int NUM_ROTARY_POSITIONS = 8;

// Durations matching positions 1 to 8:
// 1m, 2m, 3m, 5m, 10m, 15m, 15s, 30s
const unsigned long DURATION_PRESETS_MS[] = {
  60000UL,   // Pos 1: 1 min
  120000UL,  // Pos 2: 2 min
  180000UL,  // Pos 3: 3 min
  300000UL,  // Pos 4: 5 min
  600000UL,  // Pos 5: 10 min
  900000UL,  // Pos 6: 15 min
  15000UL,   // Pos 7: 15 sec
  30000UL    // Pos 8: 30 sec
};

// 2 Remote Player Audio Jacks (wired in parallel)
const int JACK_PINS[] = {A3, A4};
const int NUM_JACKS = 2;

// Main Enclosure Pushbutton (SW2)
const int MAIN_BUTTON_PIN = A2;

// Active Buzzer (BZ1)
const int BUZZER_PIN = A1;

// 5 LEDs (LED1: 10, LED2: 11, LED3: 12 [center], LED4: 13, LED5: A0)
const int LED_PINS[] = {10, 11, 12, 13, A0};
const int NUM_LEDS = 5;

// ==========================================
// STATE MACHINE & TIMING VARIABLES
// ==========================================
enum TimerState { IDLE, RUNNING, ALARM, START_ALERT, SLEEP_SWEEP, SLEEPING };
TimerState currentState = IDLE;

unsigned long totalTimerDurationMs = 60000UL;
unsigned long timeRemainingMs = 60000UL;
unsigned long lastUpdateMillis = 0;
unsigned long lastBeepMillis = 0;

// Alarm Beeper Logic (3 Beeps via DC pulse)
int beepCount = 0;
bool beepActive = false;
const int MAX_BEEPS = 3;
const unsigned long BEEP_DURATION_MS = 200;

// Start Turn Alert Logic (3 flashes + 1 low-volume click/beep)
int alertToggleCount = 0;
unsigned long lastAlertMillis = 0;
const unsigned long ALERT_HALF_PERIOD_MS = 80;
const unsigned long PLEASANT_BEEP_MS = 20; // Short DC pulse creates a pleasant low-volume click/beep

// Double-Tap Sleep Sequence Tracking
int sweepStep = 0;
unsigned long lastSweepMillis = 0;
const unsigned long SWEEP_STEP_MS = 140;

// Sleep Pulse (PWM Breathing on Pin 10)
unsigned long lastPulseMillis = 0;
int pulseBrightness = 0;
int pulseDirection = 3;

// Unified Debounce & Double-Tap Tracking
const unsigned long DEBOUNCE_DELAY = 35;       // Fast enough for snappy double clicks
const unsigned long DOUBLE_TAP_WINDOW = 380;    // Accommodates external button cable capacitance

bool lastRawMain = HIGH;
bool stableMain = HIGH;
unsigned long lastDebounceMain = 0;

bool lastRawJack[NUM_JACKS] = {HIGH, HIGH};
bool stableJack[NUM_JACKS] = {HIGH, HIGH};
unsigned long lastDebounceJack[NUM_JACKS] = {0, 0};

int clickCount = 0;
unsigned long lastClickTime = 0;

// ==========================================
// SETUP
// ==========================================
void setup() {
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    pinMode(ROTARY_PINS[i], INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_JACKS; i++) {
    pinMode(JACK_PINS[i], INPUT_PULLUP);
  }

  pinMode(MAIN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  totalTimerDurationMs = readSelectedDuration();
  timeRemainingMs = totalTimerDurationMs;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  handleInputs();
  updateTimer();
  updateAlertAnimation();
  updateSleepAnimation();
  updateLEDs();
  updateAlarm();
}

// ==========================================
// ROTARY SWITCH HELPER
// ==========================================
unsigned long readSelectedDuration() {
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    if (digitalRead(ROTARY_PINS[i]) == LOW) {
      return DURATION_PRESETS_MS[i];
    }
  }
  return DURATION_PRESETS_MS[0];
}

// ==========================================
// TIMER CONTROL LOGIC
// ==========================================
void triggerTurnStart() {
  digitalWrite(BUZZER_PIN, HIGH); // Start subtle chirp
  alertToggleCount = 0;
  lastAlertMillis = millis();

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }

  currentState = START_ALERT;
}

void triggerSleepSequence() {
  digitalWrite(BUZZER_PIN, LOW);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }

  sweepStep = 0;
  lastSweepMillis = millis();
  currentState = SLEEP_SWEEP;
}

// ==========================================
// UNIFIED BUTTON & JACK INPUT HANDLER
// ==========================================
void handleInputs() {
  unsigned long now = millis();
  bool buttonJustPressed = false;

  // 1. Read Main Button
  bool readingMain = digitalRead(MAIN_BUTTON_PIN);
  if (readingMain != lastRawMain) {
    lastDebounceMain = now;
  }
  lastRawMain = readingMain;

  if ((now - lastDebounceMain) > DEBOUNCE_DELAY) {
    if (readingMain != stableMain) {
      stableMain = readingMain;
      if (stableMain == LOW) {
        buttonJustPressed = true;
      }
    }
  }

  // 2. Read Player Audio Jacks
  for (int i = 0; i < NUM_JACKS; i++) {
    bool readingJack = digitalRead(JACK_PINS[i]);
    if (readingJack != lastRawJack[i]) {
      lastDebounceJack[i] = now;
    }
    lastRawJack[i] = readingJack;

    if ((now - lastDebounceJack[i]) > DEBOUNCE_DELAY) {
      if (readingJack != stableJack[i]) {
        stableJack[i] = readingJack;
        if (stableJack[i] == LOW) {
          buttonJustPressed = true;
        }
      }
    }
  }

  // 3. Evaluate Single vs Double Tap
  if (buttonJustPressed) {
    clickCount++;
    lastClickTime = now;

    if (clickCount >= 2) {
      clickCount = 0;
      triggerSleepSequence();
    }
  }

  if (clickCount == 1 && (now - lastClickTime > DOUBLE_TAP_WINDOW)) {
    clickCount = 0;
    triggerTurnStart();
  }
}

// ==========================================
// START-OF-TURN ALERT SEQUENCE
// ==========================================
void updateAlertAnimation() {
  if (currentState != START_ALERT) return;

  unsigned long currentMillis = millis();

  // Silence buzzer after the brief pulse
  if (currentMillis - lastAlertMillis >= PLEASANT_BEEP_MS) {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // 3 flashes = 6 edge transitions (ON/OFF x 3)
  if (currentMillis - lastAlertMillis >= ALERT_HALF_PERIOD_MS) {
    lastAlertMillis = currentMillis;
    alertToggleCount++;

    if (alertToggleCount >= 6) {
      // Flashes complete: begin active countdown
      digitalWrite(BUZZER_PIN, LOW);
      totalTimerDurationMs = readSelectedDuration();
      timeRemainingMs = totalTimerDurationMs;
      lastUpdateMillis = millis();
      currentState = RUNNING;
      return;
    }

    bool flashState = (alertToggleCount % 2 == 0);
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], flashState ? HIGH : LOW);
    }
  }
}

// ==========================================
// SLEEP ANIMATION (SWEEP OUTWARD & BREATHE)
// ==========================================
void updateSleepAnimation() {
  unsigned long currentMillis = millis();

  // Phase 1: Center-out wave
  if (currentState == SLEEP_SWEEP) {
    if (currentMillis - lastSweepMillis >= SWEEP_STEP_MS) {
      lastSweepMillis = currentMillis;
      sweepStep++;

      // Clear all LEDs
      for (int i = 0; i < NUM_LEDS; i++) digitalWrite(LED_PINS[i], LOW);

      if (sweepStep == 1) {
        // Center: LED 3 (pin 12)
        digitalWrite(LED_PINS[2], HIGH);
      } else if (sweepStep == 2) {
        // Inner Ring: LED 2 (pin 11) and LED 4 (pin 13)
        digitalWrite(LED_PINS[1], HIGH);
        digitalWrite(LED_PINS[3], HIGH);
      } else if (sweepStep == 3) {
        // Outer Ring: LED 1 (pin 10) and LED 5 (pin A0)
        digitalWrite(LED_PINS[0], HIGH);
        digitalWrite(LED_PINS[4], HIGH);
      } else {
        // Sweep finished -> Enter continuous pulsing sleep
        for (int i = 0; i < NUM_LEDS; i++) digitalWrite(LED_PINS[i], LOW);
        pulseBrightness = 0;
        pulseDirection = 3;
        currentState = SLEEPING;
      }
    }
    return;
  }

  // Phase 2: Pulse pin 10 to indicate sleeping
  if (currentState == SLEEPING) {
    if (currentMillis - lastPulseMillis >= 20) {
      lastPulseMillis = currentMillis;

      pulseBrightness += pulseDirection;
      if (pulseBrightness >= 255) {
        pulseBrightness = 255;
        pulseDirection = -pulseDirection;
      } else if (pulseBrightness <= 0) {
        pulseBrightness = 0;
        pulseDirection = -pulseDirection;
      }

      // Pin 10 has native PWM support on the Uno/Nano
      analogWrite(LED_PINS[0], pulseBrightness);
    }
  }
}

// ==========================================
// TIMER COUNTDOWN LOGIC
// ==========================================
void updateTimer() {
  if (currentState != RUNNING) return;

  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - lastUpdateMillis;
  lastUpdateMillis = currentMillis;

  if (elapsed >= timeRemainingMs) {
    timeRemainingMs = 0;
    currentState = ALARM;
    beepCount = 0;
    beepActive = false;
    lastBeepMillis = currentMillis;
  } else {
    timeRemainingMs -= elapsed;
  }
}

// ==========================================
// 5-STAGE PROGRESS & BLINK LOGIC
// ==========================================
void updateLEDs() {
  // Handled by specialized routines
  if (currentState == START_ALERT || currentState == SLEEP_SWEEP || currentState == SLEEPING) {
    return;
  }

  if (currentState == IDLE || currentState == ALARM) {
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], LOW);
    }
    return;
  }

  unsigned long elapsedMs = totalTimerDurationMs - timeRemainingMs;
  unsigned long stageDurationMs = totalTimerDurationMs / 5;

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }

  if (elapsedMs < stageDurationMs * 1) {
    digitalWrite(LED_PINS[0], HIGH);
  } else if (elapsedMs < stageDurationMs * 2) {
    digitalWrite(LED_PINS[1], HIGH);
  } else if (elapsedMs < stageDurationMs * 3) {
    digitalWrite(LED_PINS[2], HIGH);
  } else if (elapsedMs < stageDurationMs * 4) {
    digitalWrite(LED_PINS[3], HIGH);
  } else if (elapsedMs < (totalTimerDurationMs - (stageDurationMs / 2))) {
    digitalWrite(LED_PINS[4], HIGH);
  } else {
    bool blinkState = (millis() / 250) % 2 == 0;
    digitalWrite(LED_PINS[4], blinkState ? HIGH : LOW);
  }
}

// ==========================================
// ALARM SOUND LOGIC (Active DC Buzzer)
// ==========================================
void updateAlarm() {
  if (currentState != ALARM) return;

  if (beepCount >= MAX_BEEPS) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastBeepMillis >= BEEP_DURATION_MS) {
    lastBeepMillis = currentMillis;
    beepActive = !beepActive;

    if (beepActive) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      beepCount++;
    }
  }
}