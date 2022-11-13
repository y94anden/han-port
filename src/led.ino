const byte LED = 2;
const unsigned long FLASHINTERVAL = 100;
const unsigned long INTERMISSIONINTERVAL = 200;

unsigned int led_flash_count = 0;
bool led_flash_repeat = false;
static unsigned int led_current_flash = 0;
static unsigned long previousTime = 0;

void led_setup() {
  pinMode(LED, OUTPUT);
  led_flash(10, false);
}

void led_loop() {
  unsigned long diff = millis() - previousTime;

  if (led_flash_count > 0) {
    if (led_current_flash == 0) {
      // Should we restart the flashing?
      if (led_flash_repeat && diff > INTERMISSIONINTERVAL) {
        led_flash(led_flash_count, led_flash_repeat);
      }
    } else if (diff > FLASHINTERVAL) {
      led_toggle();
      previousTime += diff;
      if (digitalRead(LED) == LOW) {
        led_current_flash--;
      }
    }
  }
}

void led_toggle() { digitalWrite(LED, !digitalRead(LED)); }

void led_on() { digitalWrite(LED, LOW); }

void led_off() { digitalWrite(LED, HIGH); }

void led_flash(int count, bool repeat) {
  if (!repeat) {
    for (int i = 0; i < count; i++) {
      led_on();
      delay(FLASHINTERVAL);
      led_off();
      delay(FLASHINTERVAL);
    }
    delay(INTERMISSIONINTERVAL);
    led_flash_count = 0;
    led_current_flash = 0;
    led_flash_repeat = false;
  } else {
    led_flash_count = count;
    led_current_flash = count;
    led_flash_repeat = repeat;
    previousTime = millis();
    led_on();
  }
}
