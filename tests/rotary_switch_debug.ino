const int ROTARY_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int NUM_ROTARY_POSITIONS = 8;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    pinMode(ROTARY_PINS[i], INPUT_PULLUP);
  }
}

void loop() {
  Serial.print("Pins [D2..D9]: ");
  for (int i = 0; i < NUM_ROTARY_POSITIONS; i++) {
    Serial.print(digitalRead(ROTARY_PINS[i]));
    Serial.print(" ");
  }
  Serial.println();
  delay(300);
}
