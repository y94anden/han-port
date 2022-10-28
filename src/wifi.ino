#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti;

#define MY_SSID "network"
#define MY_PASS "password"

void wifi_setup() {
  // add Wi-Fi networks you want to connect to
  wifiMulti.addAP(MY_SSID, MY_PASS);

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    led_toggle();
  }

  led_on();
}
