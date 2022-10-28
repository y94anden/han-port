#include <ArduinoOTA.h>

#define HOSTNAME "han-port"
#define PASSWORD "VerySecretPassword"

void ota_setup() {
  // Setup login for OTA updates
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(PASSWORD);

  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) {
    } else if (error == OTA_BEGIN_ERROR) {
    } else if (error == OTA_CONNECT_ERROR) {
    } else if (error == OTA_RECEIVE_ERROR) {
    } else if (error == OTA_END_ERROR) {
    }
  });
  ArduinoOTA.begin();
}

void ota_loop() { ArduinoOTA.handle(); }
