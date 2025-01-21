#include "template-render.h"

void setup() {
  led_setup();
  gpio_setup();
  wifi_setup();
  ota_setup();
  webserver_setup();
  han_setup();
  entropy_setup();
}

void loop() {
  ota_loop();
  webserver_loop();
  led_loop();
  han_loop();
  wifi_loop();
}
