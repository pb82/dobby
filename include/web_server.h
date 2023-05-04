#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include <functional>
#include <vector>

#include "settings.h"
#include "http_parser.h"

namespace DobbyWebServer
{
    class DobbyHttpRequest;
    class DobbyHttpResponse;

    struct DobbyHttpRequest
    {
        unsigned int method;
        String url;
        String body;
        bool complete = false;
    };

    class DobbyHttpResponse
    {
    public:
        static DobbyHttpResponse OkEmptyResponse();
        static DobbyHttpResponse HtmlResponse(const char *filePath);
        static DobbyHttpResponse NotFoundResponse();

        String &toString();

    private:
        DobbyHttpResponse() {}

        String body;
    };

    void start(AppSettings &settings);
    void run(AppSettings &settings);
}

#endif