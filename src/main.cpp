#include <Arduino.h>
#include <LittleFS.h>

#include "config.h"
#include "settings.h"
#include "access_point.h"

AppSettings settings;

void setupFilesystem()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed");
    return;
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD);

  delay(1000);

  setupFilesystem();
  WiFiCredentials credentials;
  if (settings.getWiFiCredentials(&credentials))
  {
    Serial.println("wifi credentials found");
  }
  else
  {
    Serial.println("no wifi credentials found");
    AccessPoint::start();
    Serial.println("access point running");
  }
}

void loop()
{
  vTaskDelete(nullptr);
}