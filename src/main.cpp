#include <Arduino.h>
#include <LittleFS.h>
#include <Adafruit_Protomatter.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

extern "C"
{
#include <lualib.h>
#include <lauxlib.h>
#include <lua.h>
}

#include "config.h"
#include "settings.h"
#include "access_point.h"
#include "app_state.h"
#include "web_server.h"

#define DISPLAY_REFRESH_MS 1000

AppSettings settings;
// TimerHandle_t tmr_displayServer;
WebServer server(80);

Adafruit_Protomatter matrix(
    64,                        // Width of matrix (or matrix chain) in pixels
    4,                         // Bit depth, 1-6
    1, rgbPins,                // # of matrix chains, array of 6 RGB pins for each
    4, addrPins,               // # of address pins (height is inferred), array of pins
    clockPin, latchPin, oePin, // Other matrix control pins
    false);                    // No double-buffering here (see "doublebuffer" example)

void setupFilesystem()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed");
    return;
  }
}

int writeScreen(lua_State *L)
{
  const char *str = lua_tostring(L, 1);
  matrix.print(str);
  return 0;
}

int clearScreen(lua_State *L)
{
  matrix.fillScreen(matrix.color565(0, 0, 0));
  return 0;
}

int setCursor(lua_State *L)
{
  uint16_t x = lua_tointeger(L, 1);
  uint16_t y = lua_tointeger(L, 2);
  matrix.setCursor(x, y);
  return 0;
}

int wait(lua_State *L)
{
  int d = lua_tointeger(L, 1);
  vTaskDelay(pdMS_TO_TICKS(d));
  return 0;
}

void runDisplayServer(void *)
{
  lua_State *L = luaL_newstate();
  lua_register(L, "writeScreen", writeScreen);
  lua_register(L, "clearScreen", clearScreen);
  lua_register(L, "setCursor", setCursor);
  lua_register(L, "wait", wait);

  for (;;)
  {
    AppState::instance().update(DISPLAY_REFRESH_MS);
    AppState::instance().render(matrix);
    String ref;
    if (AppState::instance().getCommand(ref))
    {
      if (luaL_loadstring(L, ref.c_str()) == LUA_OK)
      {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK)
        {
          const char *err = lua_tostring(L, -1);
          Serial.println("lua error");
          if (err)
          {
            Serial.println(err);
          }
        }
      }
      else
      {
        const char *err = lua_tostring(L, -1);
        Serial.println("lua error");
        if (err)
        {
          Serial.println(err);
        }
      }
      matrix.show();
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  setupFilesystem();
  matrix.begin();

  WiFiCredentials credentials;
  if (settings.getWiFiCredentials(&credentials))
  {
    Serial.println("wifi credentials found");
    // tmr_displayServer = xTimerCreate("DISPLAY_SERVER", pdMS_TO_TICKS(DISPLAY_REFRESH_MS), true, (void *)0, runDisplayServer);
    // xTimerStart(tmr_displayServer, 0);

    xTaskCreatePinnedToCore(runDisplayServer, "DISPLAY", 4096, nullptr, 1, nullptr, 1);
    DobbyWebServer::run(settings);
  }
  else
  {
    AccessPoint::start();
    Serial.println("AP Mode");
  }
}

void loop()
{
  vTaskDelete(nullptr);
}