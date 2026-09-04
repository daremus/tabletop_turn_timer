// ==========================================
// BUTTON / JACK TEST SKETCH (Pins D10 - D13)
// ==========================================

const int BUTTON_PINS[] = {10, 11, 12, 13};
const int NUM_BUTTONS = 4;
const char* BUTTON_NAMES[] = {
  "D10 (Main Button / Player 1)",
  "D11 (Remote Player 2)",
  "D12 (Remote Player 3)",
  "D13 (Remote Player 4)"
};

// Debounce timing
const unsigned long DEBOUNCE_DELAY = 50;  // ms

bool lastRawState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH};
bool stableState[NUM_BUTTONS]  = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[NUM_BUTTONS] = {0, 0, 0, 0};

void setup() {
  Serial.begin(9600);
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  Serial.println("=============================================");
  Serial.println("   4-Button Diagnostic Test (Pins D10 - D13) ");
  Serial.println("=============================================");
  Serial.println("Initial Pin States (1 = Released / 5V, 0 = Pressed / GND):");
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    Serial.print(" - ");
    Serial.print(BUTTON_NAMES[i]);
    Serial.print(": ");
    Serial.println(digitalRead(BUTTON_PINS[i]) == LOW ? "0 (GND / PRESSED)" : "1 (OPEN / RELEASED)");
  }
  Serial.println("\nReady. Press/release any button or plug in a jack to test.\n");
}

void loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]);

    // Check if the signal bounced
    if (reading != lastRawState[i]) {
      lastDebounceTime[i] = millis();
    }
    lastRawState[i] = reading;

    // Filter out debounce jitter
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (reading != stableState[i]) {
        stableState[i] = reading;

        if (stableState[i] == LOW) {
          Serial.print("[PRESSED]  ");
          Serial.print(BUTTON_NAMES[i]);
          Serial.println(" -> Pin state: 0 (GND)");
        } else {
          Serial.print("[RELEASED] ");
          Serial.print(BUTTON_NAMES[i]);
          Serial.println(" -> Pin state: 1 (HIGH)");
        }
      }
    }
  }
}