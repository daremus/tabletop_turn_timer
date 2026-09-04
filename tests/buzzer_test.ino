// ==========================================
// STANDALONE BUZZER TEST SKETCH (Pin A1)
// ==========================================

const int BUZZER_PIN = A1;

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("=========================================");
  Serial.println("   Buzzer Diagnostic Test on Pin A1      ");
  Serial.println("=========================================");
}

void loop() {
  // Test 1: Standard 1000 Hz tone
  Serial.println("1. Playing 1000 Hz tone (500 ms)...");
  tone(BUZZER_PIN, 1000);
  delay(500);
  noTone(BUZZER_PIN);
  delay(400);

  // Test 2: Louder resonant frequency (3100 Hz)
  Serial.println("2. Playing 3100 Hz resonant peak tone (500 ms)...");
  tone(BUZZER_PIN, 3100);
  delay(500);
  noTone(BUZZER_PIN);
  delay(400);

  // Test 3: 3-Beep Alarm sequence (Simulates timer completion)
  Serial.println("3. Simulating 3-beep alarm sequence...");
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 3100);
    delay(200);
    noTone(BUZZER_PIN);
    delay(200);
  }
  delay(500);

  // Test 4: Pitch sweep (Checks passive piezo frequency response)
  Serial.println("4. Sweeping frequencies (1 kHz to 4 kHz)...");
  for (int freq = 1000; freq <= 4000; freq += 200) {
    tone(BUZZER_PIN, freq);
    delay(60);
  }
  noTone(BUZZER_PIN);
  delay(500);

  // Test 5: Direct DC pulse (Tests active buzzers)
  Serial.println("5. Testing direct DC HIGH/LOW pulse...");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\nCycle complete. Pausing for 3 seconds.\n");
  delay(3000);
}