#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti;

typedef struct {
  const char *network;
  const char *password;
} Authentication_t;

Authentication_t authentications[] = {
  // Below file contains the network ssid and passwords of the
  // networks that the chip can connect to. It should be kept
  // secret and not added to version control.
#include "authentications.secret"
};

void wifi_setup() {

  int n = sizeof(authentications) / sizeof(Authentication_t);
  if (n == 0) {
    return; // No networks configured
  }

  for (int i = 0; i < n; i++) {
    wifiMulti.addAP(authentications[i].network, authentications[i].password);
  }

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    led_toggle();
  }
}

void wifi_loop() {
  // Maintain WiFi connection
  wifiMulti.run(100);
}