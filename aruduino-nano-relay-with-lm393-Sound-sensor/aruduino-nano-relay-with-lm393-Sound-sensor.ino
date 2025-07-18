#define soundPin 8     // LM393 OUT pin
#define relayPin 7     // Relay IN pin

unsigned long lastEvent = 0;
bool relayState = false;

void setup() {
  pinMode(soundPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  Serial.begin(9600);  // Optional: for debugging
}

void loop() {
  int soundState = digitalRead(soundPin);

  if (soundState == LOW && millis() - lastEvent > 300) {
    relayState = !relayState;
    digitalWrite(relayPin, relayState ? HIGH : LOW);
    lastEvent = millis();
    Serial.println("Clap detected!");  // Optional: for debugging
  }
}
