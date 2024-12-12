#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti;

void wifi_setup() {
  // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("network1", "password1");
  wifiMulti.addAP("network2", "password2");
  wifiMulti.addAP("network3", "password3");
  wifiMulti.addAP("network4", "password4");

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    led_toggle();
  }
}

void wifi_loop() {
  // Maintain WiFi connection
  wifiMulti.run(100);
}