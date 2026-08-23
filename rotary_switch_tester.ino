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

// 4 Remote Player Audio Jacks (J1, J2, J3, J4)
const int JACK_PINS[] = {10, 11, 12, 13};
const int NUM_JACKS = 4;

// Main Enclosure Pushbutton (SW2)
const int MAIN_BUTTON_PIN = A1;

// Piezo Buzzer (BZ1)
const int BUZZER_PIN = A0;

// 5 LEDs (LED1 to LED5)
const int LED_PINS[] = {A3, A4, A5, A6, A7};
const int NUM_LEDS = 5;

// ==========================================
// STATE MACHINE & TIMING VARIABLES
// ==========================================
enum TimerState { IDLE, RUNNING, PAUSED, ALARM };
TimerState currentState = IDLE;

unsigned long totalTimerDurationMs = 60000UL;
unsigned long timeRemainingMs = 60000UL;
unsigned long lastUpdateMillis = 0;
unsigned long lastBeepMillis = 0;

// Diagnostic Tracker for Rotary Serial Output
int lastReportedPos = -1;

// Alarm Beeper Logic (3 Tones)
int beepCount = 0;
bool beepActive = false;
const int MAX_BEEPS = 3;
const unsigned long BEEP_DURATION_MS = 200;

// Button Debouncing & Double-Tap Logic (Main Button SW2)
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long DOUBLE_TAP_GAP = 300;

bool lastRawMainBtn = HIGH;
bool stableMainBtn = HIGH;
unsigned long lastDebounceMain = 0;
unsigned long lastReleaseTime = 0;
int tapCount = 0;

// Remote Audio Jack Debounce States
bool lastJackState[NUM_JACKS] = {HIGH, HIGH, HIGH, HIGH};
bool stableJackState[NUM_JACKS] = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastJackDebounce[NUM_JACKS] = {0, 0, 0, 0};

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(9600);

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
  checkRotarySwitchPosition();
  handleMainButton();
  handlePlayerJacks();
  updateTimer();
  updateLEDs();
  updateAlarm();
}

// ==========================================
// ROTARY SWITCH HELPERS
// ==========================================
void checkRotarySwitchPosition() {
  int currentPos = -1;
  int activePin = -1;

  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    if (digitalRead(ROTARY_PINS[i]) == LOW) {
      currentPos = i + 1; // 1 to 8
      activePin = ROTARY_PINS[i];
      break;
    }
  }

  if (currentPos != lastReportedPos) {
    lastReportedPos = currentPos;
    if (currentPos != -1) {
      Serial.print("Rotary Switch Position: ");
      Serial.print(currentPos);
      Serial.print(" | Arduino Pin: D");
      Serial.print(activePin);
      Serial.print(" | Duration: ");
      Serial.print(DURATION_PRESETS_MS[currentPos - 1] / 1000);
      Serial.println("s");
    } else {
      Serial.println("Switch is between notches or not grounded (All pins HIGH).");
    }
  }
}

unsigned long readSelectedDuration() {
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    if (digitalRead(ROTARY_PINS[i]) == LOW) {
      return DURATION_PRESETS_MS[i];
    }
  }
  return DURATION_PRESETS_MS[0]; // Default to Pos 1 (1 min) if between notches
}

// ==========================================
// TIMER START / RESET / PAUSE LOGIC
// ==========================================
void startOrResetTimer() {
  noTone(BUZZER_PIN);
  totalTimerDurationMs = readSelectedDuration(); // Update to current rotary setting
  timeRemainingMs = totalTimerDurationMs;
  lastUpdateMillis = millis();
  currentState = RUNNING;
}

void togglePause() {
  if (currentState == RUNNING) {
    currentState = PAUSED;
  } else if (currentState == PAUSED) {
    lastUpdateMillis = millis();
    currentState = RUNNING;
  }
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
      togglePause();
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
// ALARM SOUND LOGIC (3 Tones)
// ==========================================
void updateAlarm() {
  if (currentState != ALARM) return;

  if (beepCount >= MAX_BEEPS) {
    noTone(BUZZER_PIN);
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastBeepMillis >= BEEP_DURATION_MS) {
    lastBeepMillis = currentMillis;
    beepActive = !beepActive;

    if (beepActive) {
      tone(BUZZER_PIN, 1000); // 1 kHz beep tone
    } else {
      noTone(BUZZER_PIN);
      beepCount++;
    }
  }
}
