#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

#include <freertos/semphr.h>

struct WiFiCredentials {
    String ssid;
    String password;

    void fromUrlEncoded(const char *encoded);
};

class AppSettings {
public:
    AppSettings();

    void resetAllSettings();
    bool getWiFiCredentials(WiFiCredentials *target);
    void setWiFiCredentials(WiFiCredentials *from);

    static SemaphoreHandle_t mutex;

private:
    Preferences prefs;
};

#endif