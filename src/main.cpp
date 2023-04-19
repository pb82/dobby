#include <Arduino.h>

#include "config.h"
#include "settings.h"

AppSettings settings;

void setup() {
  Serial.begin(SERIAL_BAUD);

  delay(1000);

  WiFiCredentials credentials;
  if (settings.getWiFiCredentials(&credentials)) {
    Serial.println("wifi credentials found");
  } else {
    Serial.println("no wifi credentials found");
  }
}

void loop() {
  vTaskDelete(nullptr);
}