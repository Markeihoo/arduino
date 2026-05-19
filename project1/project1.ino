const int GREEN_PIN = 12;

void setup() {
  pinMode(GREEN_PIN, OUTPUT);
}

void loop() {
  digitalWrite(GREEN_PIN, HIGH);
  delay(1000);
  digitalWrite(GREEN_PIN, LOW);
  delay(1000);
}
