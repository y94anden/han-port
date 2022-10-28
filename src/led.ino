const byte LED = 2;
const unsigned long INTERVAL = 1000;

void led_setup() {
  pinMode(LED, OUTPUT);
}

void led_loop() {
  static unsigned long previousTime = 0;
  unsigned long diff = millis() - previousTime;
  if(diff > INTERVAL) {
    led_toggle();
    previousTime += diff;
  }
}

void led_toggle() {
  digitalWrite(LED, !digitalRead(LED));
}

void led_on() {
  digitalWrite(LED, LOW);
}

void led_off() {
  digitalWrite(LED, HIGH);
}
