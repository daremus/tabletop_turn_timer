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

// 2 Remote Player Audio Jacks (originally each is wired with two jacks in parallel)
const int JACK_PINS[] = {A3, A4};
const int NUM_JACKS = 2;

// Main Enclosure Pushbutton (SW2)
const int MAIN_BUTTON_PIN = A2;

// Active Buzzer (BZ1)
const int BUZZER_PIN = A1;

// 5 LEDs (LED1 to LED5)
const int LED_PINS[] = {10, 11, 12, 13, A0};
const int NUM_LEDS = 5;

// ==========================================
// STATE MACHINE & TIMING VARIABLES
// ==========================================
enum TimerState { IDLE, RUNNING, STOPPING, ALARM };
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

// Double-Tap Stop & LED Flash Tracking
int flashTargetLED = 0;
int flashToggleCount = 0;
bool flashLEDState = false;
unsigned long lastFlashMillis = 0;
const unsigned long FLASH_HALF_PERIOD_MS = 150; // 150ms ON, 150ms OFF

// Button Debouncing & Double-Tap Logic (Main Button SW2)
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long DOUBLE_TAP_GAP = 300;

bool lastRawMainBtn = HIGH;
bool stableMainBtn = HIGH;
unsigned long lastDebounceMain = 0;
unsigned long lastReleaseTime = 0;
int tapCount = 0;

// Remote Audio Jack Debounce States
bool lastJackState[NUM_JACKS] = {HIGH, HIGH};
bool stableJackState[NUM_JACKS] = {HIGH, HIGH};
unsigned long lastJackDebounce[NUM_JACKS] = {0, 0};

// ==========================================
// SETUP
// ==========================================
void setup() {
  // Initialize Rotary Switch Pins
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    pinMode(ROTARY_PINS[i], INPUT_PULLUP);
  }

  // Initialize Remote Player Audio Jacks
  for (int i = 0; i < NUM_JACKS; i++) {
    pinMode(JACK_PINS[i], INPUT_PULLUP);
  }

  // Initialize Main Button, Buzzer, and LEDs
  pinMode(MAIN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  // Read initial duration from rotary switch
  totalTimerDurationMs = readSelectedDuration();
  timeRemainingMs = totalTimerDurationMs;
  updateLEDs();
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  handleMainButton();
  handlePlayerJacks();
  updateTimer();
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
  return DURATION_PRESETS_MS[0]; // Default to 1 min if between notches
}

// ==========================================
// TIMER START / RESET / STOP LOGIC
// ==========================================
void startOrResetTimer() {
  digitalWrite(BUZZER_PIN, LOW); // Silence active buzzer
  totalTimerDurationMs = readSelectedDuration(); // Update to current rotary setting
  timeRemainingMs = totalTimerDurationMs;
  lastUpdateMillis = millis();
  currentState = RUNNING;
}

void triggerStopSequence() {
  if (currentState == RUNNING) {
    // Determine which LED is currently active based on elapsed time
    unsigned long elapsedMs = totalTimerDurationMs - timeRemainingMs;
    unsigned long stageDurationMs = totalTimerDurationMs / 5;
    flashTargetLED = elapsedMs / stageDurationMs;
    if (flashTargetLED >= NUM_LEDS) flashTargetLED = NUM_LEDS - 1;
  } else {
    flashTargetLED = 0;
  }

  // Turn off buzzer and clear all other LEDs
  digitalWrite(BUZZER_PIN, LOW);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }

  // Prime the 10-flash sequence (20 toggles: 10 ON, 10 OFF)
  flashToggleCount = 0;
  flashLEDState = true;
  digitalWrite(LED_PINS[flashTargetLED], HIGH);
  lastFlashMillis = millis();
  currentState = STOPPING;
}

// ==========================================
// INPUT HANDLERS
// ==========================================
void handleMainButton() {
  bool reading = digitalRead(MAIN_BUTTON_PIN);

  if (reading != lastRawMainBtn) {
    lastDebounceMain = millis();
  }
  lastRawMainBtn = reading;

  if ((millis() - lastDebounceMain) > DEBOUNCE_DELAY) {
    if (reading != stableMainBtn) {
      stableMainBtn = reading;

      // Active LOW press
      if (stableMainBtn == LOW) {
        tapCount++;
        if (tapCount == 1) {
          lastReleaseTime = millis();
        }
      }
    }
  }

  // Handle single vs double tap
  if (tapCount > 0) {
    if (tapCount == 2) {
      triggerStopSequence();
      tapCount = 0;
    } else if (millis() - lastReleaseTime > DOUBLE_TAP_GAP) {
      startOrResetTimer();
      tapCount = 0;
    }
  }
}

void handlePlayerJacks() {
  for (int i = 0; i < NUM_JACKS; i++) {
    bool reading = digitalRead(JACK_PINS[i]);

    if (reading != lastJackState[i]) {
      lastJackDebounce[i] = millis();
    }
    lastJackState[i] = reading;

    if ((millis() - lastJackDebounce[i]) > DEBOUNCE_DELAY) {
      if (reading != stableJackState[i]) {
        stableJackState[i] = reading;

        // Player pressed end-of-turn button
        if (stableJackState[i] == LOW) {
          startOrResetTimer();
        }
      }
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
  // Flash current LED 10 times then enter IDLE
  if (currentState == STOPPING) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastFlashMillis >= FLASH_HALF_PERIOD_MS) {
      lastFlashMillis = currentMillis;
      flashToggleCount++;

      if (flashToggleCount >= 20) { // 20 toggles = exactly 10 full flashes
        digitalWrite(LED_PINS[flashTargetLED], LOW);
        currentState = IDLE;
        return;
      }

      flashLEDState = !flashLEDState;
      digitalWrite(LED_PINS[flashTargetLED], flashLEDState ? HIGH : LOW);
    }
    return;
  }

  if (currentState == IDLE || currentState == ALARM) {
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], LOW);
    }
    return;
  }

  unsigned long elapsedMs = totalTimerDurationMs - timeRemainingMs;
  unsigned long stageDurationMs = totalTimerDurationMs / 5; // Divides any duration into 5 equal steps

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }

  if (elapsedMs < stageDurationMs * 1) {
    // Stage 1 (0–20%): LED 1 ON
    digitalWrite(LED_PINS[0], HIGH);
  } else if (elapsedMs < stageDurationMs * 2) {
    // Stage 2 (20–40%): LED 2 ON
    digitalWrite(LED_PINS[1], HIGH);
  } else if (elapsedMs < stageDurationMs * 3) {
    // Stage 3 (40–60%): LED 3 ON
    digitalWrite(LED_PINS[2], HIGH);
  } else if (elapsedMs < stageDurationMs * 4) {
    // Stage 4 (60–80%): LED 4 ON
    digitalWrite(LED_PINS[3], HIGH);
  } else if (elapsedMs < (totalTimerDurationMs - (stageDurationMs / 2))) {
    // Stage 5a (80–90%): LED 5 Solid ON
    digitalWrite(LED_PINS[4], HIGH);
  } else {
    // Stage 5b (Final 10% of time): LED 5 Blinks every 1/4 second
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
      digitalWrite(BUZZER_PIN, HIGH); // DC High to turn active buzzer ON
    } else {
      digitalWrite(BUZZER_PIN, LOW);  // DC Low to turn active buzzer OFF
      beepCount++;
    }
  }
}