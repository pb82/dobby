#include "access_point.h"
#include "settings.h"

#include <http_parser.h>

#include <DNSServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/task.h>
#include <vector>

namespace AccessPoint
{
    DNSServer dns;
    TaskHandle_t task;
    WiFiServer server(80);
    AppSettings settings;
    std::vector<String> networks;
    http_parser_settings httpSettings;

    struct HttpResponse
    {
        HttpResponse(unsigned int status, const char *contentType, String &body) : status(status)
        {
            this->body.concat("HTTP/1.1 ");
            this->body.concat(status);
            this->body.concat(" ");
            this->body.concat(statusText());
            this->body.concat("\r\n");
            this->body.concat("Content-Type: ");
            this->body.concat(contentType);
            this->body.concat("\r\n");
            this->body.concat("Content-Length: ");
            this->body.concat(body.length());
            this->body.concat("\r\n");
            this->body.concat("\r\n");
            this->body.concat(body);
        }

        const char *statusText()
        {
            switch (this->status)
            {
            case 200:
                return "OK";
            default:
                return "unknown";
            }
        }

        unsigned int status;
        String body;
    };

    struct HttpRequest
    {
        HttpRequest()
        {
            this->complete = false;
        }
        String url;
        String body;
        unsigned int method;
        bool complete;
    };

    void urldecode2(char *dst, const char *src)
    {
        char a, b;
        while (*src)
        {
            if ((*src == '%') &&
                ((a = src[1]) && (b = src[2])) &&
                (isxdigit(a) && isxdigit(b)))
            {
                if (a >= 'a')
                    a -= 'a' - 'A';
                if (a >= 'A')
                    a -= ('A' - 10);
                else
                    a -= '0';
                if (b >= 'a')
                    b -= 'a' - 'A';
                if (b >= 'A')
                    b -= ('A' - 10);
                else
                    b -= '0';
                *dst++ = 16 * a + b;
                src += 3;
            }
            else if (*src == '+')
            {
                *dst++ = ' ';
                src++;
            }
            else
            {
                *dst++ = *src++;
            }
        }
        *dst++ = '\0';
    }

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

    int onUrl(http_parser *parser, const char *at, size_t length)
    {
        String url(at, length);
        ((HttpRequest *)parser->data)->url = url;
        ((HttpRequest *)parser->data)->method = parser->method;
        return 0;
    }

    int onMessageComplete(http_parser *parser)
    {
        HttpRequest *req = (HttpRequest *)parser->data;
        req->complete = true;
        return 0;
    }

    int onBody(http_parser *parser, const char *at, size_t length)
    {
        HttpRequest *req = (HttpRequest *)parser->data;
        req->body.concat(at, length);
        return 0;
    }

    void process(void *)
    {
        char buffer[256];
        for (;;)
        {
            dns.processNextRequest();

            WiFiClient client = server.available();
            if (client)
            {
                HttpRequest request;
                http_parser parser;
                parser.data = &request;
                http_parser_init(&parser, HTTP_REQUEST);
                if (client.connected())
                {
                    while (client.available())
                    {
                        size_t bytesRead = client.readBytes(buffer, 256);
                        http_parser_execute(&parser, &httpSettings, buffer, bytesRead);
                    }
                    if (request.complete)
                    {
                        if (request.method == HTTP_GET && request.url.compareTo("/networks") == 0)
                        {
                            String contents;
                            contents.concat("[");
                            for (int i = 0; i < networks.size(); i++)
                            {
                                contents.concat("\"");
                                contents.concat(networks.at(i));
                                contents.concat("\"");
                                if (i < networks.size() - 1)
                                {
                                    contents.concat(",");
                                }
                            }
                            contents.concat("]");
                            HttpResponse response(200, "application/json", contents);
                            client.write(response.body.c_str());
                            client.println();
                        }
                        else if (request.method == HTTP_GET)
                        {
                            String contents;
                            readFile(LittleFS, "/setup.html", contents);
                            HttpResponse response(200, "text/html", contents);
                            client.write(response.body.c_str());
                            client.println();
                        }
                        else if (request.method == HTTP_POST)
                        {
                            char *decoded = (char *)malloc(request.body.length() + 1);
                            urldecode2(decoded, request.body.c_str());

                            WiFiCredentials creds;
                            creds.fromUrlEncoded(decoded);
                            settings.setWiFiCredentials(&creds);

                            String contents;
                            contents.concat("<html><body><h1>Thanks!</h1></body></html>");
                            HttpResponse response(200, "text/html", contents);
                            client.write(response.body.c_str());
                            client.println();
                            delay(1000);
                            ESP.restart();
                        }
                    }
                }
            }
        }
    }

    void scan()
    {
        networks.clear();
        int16_t networksFound = WiFi.scanNetworks();
        for (int16_t i = 0; i < networksFound; i++)
        {
            networks.emplace_back(WiFi.SSID(i));
        }
    }

    void start()
    {
        httpSettings.on_url = onUrl;
        httpSettings.on_message_complete = onMessageComplete;
        httpSettings.on_body = onBody;

        scan();
        WiFi.softAP("Dobby_AP");
        dns.start(53, "*", WiFi.softAPIP());
        server.begin();

        xTaskCreate(process, "AP_LOOP", 4096, nullptr, 1, &task);
    }
} // namespace AccessPoint