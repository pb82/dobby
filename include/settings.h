#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

struct WiFiCredentials {
    const char *ssid;
    const char *password;
};

class AppSettings {
public:
    bool hasWiFiCredentials();
    void resetAllSettings();
    bool getWiFiCredentials(WiFiCredentials *target);
private:
    Preferences prefs;
};

#endif