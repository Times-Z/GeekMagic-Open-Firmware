#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <functional>

/**
 * @brief HTTP status code 200
 */
static int constexpr HTTP_CODE_OK = 200;

/**
 * @brief HTTP status code 404
 */
static int constexpr HTTP_CODE_NOT_FOUND = 404;

/**
 * @brief HTTP status code 500
 */
static int constexpr HTTP_CODE_INTERNAL_ERROR = 500;

class Webserver {
   public:
    explicit Webserver(uint16_t port = 80);
    static auto beginFS(bool formatIfFailed = false) -> bool;
    void begin();
    void handleClient();
    void on(const String& uri, HTTPMethod method, std::function<void()> handler);
    void on(const String& uri, std::function<void()> handler);
    void serveStatic(const String& uri, const String& path, const String& contentType = String(),
                     int cacheSeconds = 86400, bool tryGzip = true);
    void onNotFound(std::function<void()> handler);
    ESP8266WebServer& raw();

   private:
    ESP8266WebServer _server;
    static String guessContentType(const String& path);
};

#endif  // WEB_SERVER_H
