const byte GPIO_PIN = 0; // pin 5

void gpio_setup() {
  pinMode(GPIO_PIN, OUTPUT);
  gpio_off();
}

void gpio_on() { digitalWrite(GPIO_PIN, HIGH); }

void gpio_off() { digitalWrite(GPIO_PIN, LOW); }

bool gpio_state() { return digitalRead(GPIO_PIN) == HIGH; }
