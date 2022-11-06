const byte LED = 2;
const unsigned long OFFINTERVAL = 5000;
const unsigned long ONINTERVAL = 100;

void led_setup() { pinMode(LED, OUTPUT); }

void led_loop() {
  /*
  static unsigned long previousTime = 0;
  unsigned long diff = millis() - previousTime;
  unsigned long interval = digitalRead(LED) ? ONINTERVAL : OFFINTERVAL;
  if (diff > interval) {
    led_toggle();
    previousTime += diff;
  }
  */
}

void led_toggle() { digitalWrite(LED, !digitalRead(LED)); }

void led_on() { digitalWrite(LED, LOW); }

void led_off() { digitalWrite(LED, HIGH); }
