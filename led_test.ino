// ==========================================
// 5-LED DIAGNOSTIC TEST SKETCH (Pins A1 - A5)
// ==========================================

const int LED_PINS[] = {A1, A2, A3, A4, A5};
const int NUM_LEDS = 5;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting LED Diagnostic Test on Pins A1 - A5...");

  // Initialize all 5 analog pins as digital outputs and turn them off
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
}

void loop() {
  // Test 1: Sequential Chase (One LED at a time)
  Serial.println("Running sequential chase (1 to 5)...");
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    Serial.print("LED ");
    Serial.print(i + 1);
    Serial.print(" (Pin A");
    Serial.print(i + 1);
    Serial.println(") ON");
    delay(400);
    digitalWrite(LED_PINS[i], LOW);
  }
  delay(300);

  // Test 2: Progress Bar Fill-Up (1 by 1 ON, then all OFF)
  Serial.println("Building progress bar...");
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    delay(300);
  }
  delay(500);

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
  delay(300);

  // Test 3: Blink All LEDs Simultaneously (3 times)
  Serial.println("Blinking all LEDs 3 times...");
  for (int b = 0; b < 3; b++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], HIGH);
    }
    delay(200);
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], LOW);
    }
    delay(200);
  }

  Serial.println("Cycle complete. Pausing for 2 seconds.\n");
  delay(2000);
}