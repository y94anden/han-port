
void setup() {
  led_setup();
  wifi_setup();
  ota_setup();
  webserver_setup();
  han_setup();
}

void loop() {
  ota_loop();
  webserver_loop();
  led_loop();
  han_loop();
}
