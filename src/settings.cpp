#include "settings.h"

SemaphoreHandle_t AppSettings::mutex = xSemaphoreCreateMutex();

AppSettings::AppSettings()
{
    xSemaphoreGive(this->mutex);
}

bool AppSettings::getWiFiCredentials(WiFiCredentials *target)
{
    bool result = false;
    if (xSemaphoreTake(this->mutex, 0))
    {
        prefs.begin("wifi");
        if (prefs.isKey("ssid") && prefs.isKey("password"))
        {
            target->ssid = prefs.getString("ssid");
            target->password = prefs.getString("password");
            result = true;
        }
        prefs.end();
        xSemaphoreGive(this->mutex);
    }
    return result;
}

void AppSettings::setWiFiCredentials(WiFiCredentials *from)
{
    if (xSemaphoreTake(this->mutex, 0))
    {
        this->prefs.begin("wifi");
        this->prefs.putString("ssid", from->ssid);
        this->prefs.putString("password", from->password);
        this->prefs.end();
        xSemaphoreGive(this->mutex);
    }
}

void AppSettings::resetAllSettings()
{
    prefs.begin("wifi");
    prefs.clear();
    prefs.end();
}

void WiFiCredentials::fromUrlEncoded(const char *encoded)
{
    char buffer[64];
    memset(buffer, '\0', 64);
    int len = strlen(encoded);
    int bufferPos = 0;
    for (int i = 0; i < len; i++)
    {
        if (encoded[i] == '=')
        {
            memset(buffer, '\0', 64);
            bufferPos = 0;
            continue;
        }
        if (encoded[i] == '&')
        {
            this->ssid = String(buffer);
            memset(buffer, '\0', 64);
            bufferPos = 0;
            continue;
        }
        buffer[bufferPos] = encoded[i];
        bufferPos++;
    }

    this->password = String(buffer);
}