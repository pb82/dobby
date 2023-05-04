#include "web_server.h"
#include "app_state.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <freertos/task.h>

namespace DobbyWebServer
{
    const char *HTTP_RESP_200 = "HTTP/1.1 200 OK\r\n";
    const char *HTTP_RESP_404 = "HTTP/1.1 404 Not Found\r\n";
    const char *HTTP_RESP_204 = "HTTP/1.1 204 No Content\r\n";

    http_parser_settings httpParserSettings;

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

    DobbyHttpResponse DobbyHttpResponse::HtmlResponse(const char *filePath)
    {
        DobbyHttpResponse resp;
        resp.body = HTTP_RESP_200;
        String fileBuffer;
        readFile(LittleFS, filePath, fileBuffer);
        resp.body.concat("Content-Type: text/html; charset=UTF-8\r\n");
        resp.body.concat("Content-Length: ");
        resp.body.concat(fileBuffer.length());
        resp.body.concat("\r\n");
        resp.body.concat("\r\n");
        resp.body.concat(fileBuffer);
        return resp;
    }

    DobbyHttpResponse DobbyHttpResponse::NotFoundResponse()
    {
        DobbyHttpResponse resp;
        resp.body = HTTP_RESP_404;
        return resp;
    }

    DobbyHttpResponse DobbyHttpResponse::OkEmptyResponse()
    {
        DobbyHttpResponse resp;
        resp.body = HTTP_RESP_204;
        return resp;
    }

    String &DobbyHttpResponse::toString()
    {
        return this->body;
    }

    int onUrl(http_parser *parser, const char *at, size_t length)
    {
        ((DobbyHttpRequest *)parser->data)->url = String(at, length);
        return 0;
    }

    int onBody(http_parser *parser, const char *at, size_t length)
    {
        ((DobbyHttpRequest *)parser->data)->body = String(at, length);
        return 0;
    }

    int onMessageComplete(http_parser *parser)
    {
        ((DobbyHttpRequest *)parser->data)->complete = true;
        return 0;
    }

    void start(void *args)
    {
        httpParserSettings.on_url = onUrl;
        httpParserSettings.on_message_complete = onMessageComplete;
        httpParserSettings.on_body = onBody;

        AppSettings *settings = (AppSettings *)args;
        WiFiCredentials credentials;
        if (settings->getWiFiCredentials(&credentials))
        {
            WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }

            Serial.println("WiFi connected");

            if (!MDNS.begin("dobby"))
            {
                return;
            }

            WiFiServer server(80);
            server.begin();
            server.setNoDelay(true);
            MDNS.addService("http", "tcp", 80);
            char buffer[1024];

            for (;;)
            {
                WiFiClient client = server.available();
                if (!client)
                {
                    continue;
                }

                DobbyHttpRequest request;
                http_parser parser;
                parser.data = &request;
                http_parser_init(&parser, HTTP_REQUEST);

                if (client.connected())
                {
                    client.setTimeout(1);
                    memset(buffer, '\0', 1024);
                    while (client.available()) {
                        size_t read = client.readBytes(buffer, 1024);
                        http_parser_execute(&parser, &httpParserSettings, buffer, read);
                    }

                    if (request.complete)
                    {
                        if (request.url.compareTo("/") == 0)
                        {
                            auto r = DobbyHttpResponse::HtmlResponse("/repl.html");
                            client.write(r.toString().c_str());
                        }
                        else if (request.url.compareTo("/submit") == 0)
                        {
                            AppState::instance().addCommand(std::make_shared<LuaCommand>(request.body));
                            auto r = DobbyHttpResponse::OkEmptyResponse();
                            client.write(r.toString().c_str());    
                            Serial.println("command added to queue");                    
                        }
                        else
                        {
                            auto r = DobbyHttpResponse::NotFoundResponse();
                            client.write(r.toString().c_str());
                        }
                        client.flush();
                    }                    
                }
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        return;
    }

    void run(AppSettings &settings)
    {
        xTaskCreate(start, "WEB", 4096, &settings, 1, nullptr);
    }

}