// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GeekMagic Open Firmware
 * Copyright (C) 2026 Times-Z
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <functional>
#include <vector>

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
    void serveStaticC(const char* uriC, const char* pathC, const char* contentTypeC = nullptr, int cacheSeconds = 86400,
                      bool tryGzip = true);
    void registerStaticDir(const String& fsDir, const String& uriPrefix, const String& contentType);
    void onNotFound(std::function<void()> handler);
    ESP8266WebServer& raw();
    ~Webserver();

   private:
    ESP8266WebServer _server;
    // Holds allocated C-strings for static route registrations so
    // lambdas can capture small pointers instead of copying Strings.
    std::vector<char*> _staticAllocations;

    // Periodic heap logging state
    unsigned long _lastHeapLogMillis;
    unsigned long _heapLogIntervalMs;
    unsigned long _initialFreeHeap;

    void logHeapIfNeeded();

    static String guessContentType(const String& path);
};

#endif  // WEB_SERVER_H
