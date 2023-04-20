#include "access_point.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <freertos/task.h>

namespace AccessPoint
{

    void readFile(fs::FS &fs, const char *path, String &result);

    class CaptiveHandler : public AsyncWebHandler
    {
    public:
        virtual ~CaptiveHandler() {}

        bool canHandle(AsyncWebServerRequest *req) override
        {
            return true;
        }

        void handleRequest(AsyncWebServerRequest *req)
        {

            String contents;
            readFile(LittleFS, "/setup.html", contents);
            req->send(200, "text/html", contents);
        }
    };

    DNSServer dns;
    AsyncWebServer server(80);
    CaptiveHandler handler;
    TaskHandle_t task;

    void readFile(fs::FS &fs, const char *path, String &result)
    {
        File file = fs.open(path);
        if (!file || file.isDirectory())
        {
            return;
        }

        while (file.available())
        {
            result.concat(file.readString());
        }
        file.close();
    }

    void process(void *)
    {
        for (;;)
        {
            dns.processNextRequest();
        }
    }

    void start()
    {
        WiFi.softAP("Dobby_AP");
        bool success = dns.start(53, "*", WiFi.softAPIP());
        server.addHandler(&handler).setFilter(ON_AP_FILTER);
        server.on("/networks", [](AsyncWebServerRequest *req)
                  { req->send(200, "application/json", "{\"networks\": [\"A_Network_SSID\", \"B_Network_SSID\"]}"); });
        xTaskCreate(process, "AP_LOOP", 4096, nullptr, 1, &task);
        server.begin();
    }

    void stop()
    {
        vTaskDelete(&task);
        server.end();
        dns.stop();
    }

} // namespace AccessPoint