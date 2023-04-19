#include "settings.h"

bool AppSettings::hasWiFiCredentials()
{
    bool result = false;
    prefs.begin("wifi");
    if (prefs.isKey("ssid") && prefs.isKey("password"))
    {
        result = true;
    }
    prefs.end();
    return result;
}