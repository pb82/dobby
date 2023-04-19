#include "settings.h"

bool AppSettings::getWiFiCredentials(WiFiCredentials *target) {
    bool result = false;
    prefs.begin("wifi");
    if (prefs.isKey("ssid") && prefs.isKey("password")) {
        target->ssid = prefs.getString("ssid");
        target->password = prefs.getString("password");
        result = true;
    }
    prefs.end();
    return result;
}

void AppSettings::resetAllSettings() {
    prefs.clear();
}