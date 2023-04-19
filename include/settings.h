#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

struct WiFiCredentials {
    String ssid;
    String password;
};

class AppSettings {
public:
    void resetAllSettings();
    bool getWiFiCredentials(WiFiCredentials *target);
private:
    Preferences prefs;
};

#endif