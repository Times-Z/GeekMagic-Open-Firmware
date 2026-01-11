#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <functional>
#include <Logger.h>

#include "web/Webserver.h"

/**
 * @brief Construct a new Webserver object
 * @param port Port number to listen on
 *
 * @return void
 */
Webserver::Webserver(uint16_t port) : _server(port) {}

/**
 * @brief Initializes the LittleFS filesystem
 * @param formatIfFailed Whether to format the filesystem if mounting fails
 *
 * @return true if filesystem is mounted false otherwise
 */
auto Webserver::beginFS(bool formatIfFailed) -> bool {
    if (LittleFS.begin()) {
        return true;
    };

    if (formatIfFailed) {
        return LittleFS.begin();
    };

    return false;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
/**
 * @brief Starts the webserver
 *
 * @return void
 */
void Webserver::begin() {
    Logger::info("Starting webserver", "Webserver");
    _server.begin();
}
// NOLINTEND(readability-convert-member-functions-to-static)

/**
 * @brief Handles incoming client requests
 *
 * @return void
 */
void Webserver::handleClient() { _server.handleClient(); }

/**
 * @brief Register a handler for a route
 * @param uri The URI path to handle
 * @param method The HTTP method to handle
 * @param handler The function to call when the route is accessed
 *
 * @return void
 */
void Webserver::on(const String& uri, HTTPMethod method, std::function<void()> handler) {
    _server.on(uri.c_str(), method, [handler]() { handler(); });
}

/**
 * @brief Register a generic handler (all methods)
 * @param uri The URI path to handle
 * @param handler The function to call when the route is accessed
 *
 * @return void
 */
void Webserver::on(const String& uri, std::function<void()> handler) {
    _server.on(uri.c_str(), [handler]() { handler(); });
}

/**
 * @brief Serve a static file from LittleFS. If a .gz variant exists, serve it with gzip encoding.
 * @param uri The URL path (e.g. "/index.html")
 * @param path The filesystem path (e.g. "/www/index.html")
 * @param contentType The content type to use. If empty, it will be derived from the file extension.
 * @param cacheSeconds The number of seconds to cache the file (0 = no-cache)
 * @param tryGzip Whether to try serving a .gz variant if it exists
 *
 * @return void
 */
void Webserver::serveStatic(const String& uri, const String& path, const String& contentType, int cacheSeconds,
                            bool tryGzip) {
    _server.on(uri.c_str(), HTTP_GET, [this, path, contentType, cacheSeconds, tryGzip, uri]() {
        String fsPath = path;
        String enc = "";
        String servePath = fsPath;

        if (tryGzip) {
            String gzPath = fsPath + String(".gz");
            if (LittleFS.exists(gzPath)) {
                servePath = gzPath;
                enc = "gzip";
            }
        }

        if (!LittleFS.exists(servePath)) {
            Logger::error(("File not found: " + servePath).c_str(), "Webserver");
            _server.send(HTTP_CODE_NOT_FOUND, "text/plain", "Not found");

            return;
        }

        File f = LittleFS.open(servePath, "r");
        if (!f) {
            Logger::error(("Failed to open file: " + servePath).c_str(), "Webserver");
            _server.send(HTTP_CODE_INTERNAL_ERROR, "text/plain", "Open failed");

            return;
        }

        size_t size = f.size();

        String ct = contentType;
        if (ct.length() == 0) ct = guessContentType(fsPath);

        if (cacheSeconds > 0) {
            _server.sendHeader("Cache-Control", String("public, max-age=") + String(cacheSeconds));
        } else {
            _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        }

        if (enc.length()) {
            _server.sendHeader("Content-Encoding", enc);
        }

        _server.setContentLength(size);
        _server.streamFile(f, ct);
        f.close();

        Logger::info(("Served " + servePath + " for URI: " + uri).c_str(), "Webserver");
    });
}

/**
 * @brief Simple notFound handler registration
 * @param handler The function to call when a route is not found
 *
 * @return void
 */
void Webserver::onNotFound(std::function<void()> handler) {
    _server.onNotFound([handler]() { handler(); });
}

/**
 * @brief Expose underlying server where advanced config is needed
 *
 * @return reference to the underlying ESP8266WebServer
 */
auto Webserver::raw() -> ESP8266WebServer& { return _server; }

/**
 * @brief Guess the content type based on the file extension
 * @param path The file path
 *
 * @return The guessed content type
 */
auto Webserver::guessContentType(const String& path) -> String {
    if (path.endsWith(".html") || path.endsWith(".htm") || path.endsWith("/")) {
        return "text/html";
    }

    if (path.endsWith(".css")) {
        return "text/css";
    }

    if (path.endsWith(".js")) {
        return "application/javascript";
    }

    if (path.endsWith(".json")) {
        return "application/json";
    }

    if (path.endsWith(".png")) {
        return "image/png";
    }

    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) {
        return "image/jpeg";
    }

    if (path.endsWith(".gif")) {
        return "image/gif";
    }

    if (path.endsWith(".svg")) {
        return "image/svg+xml";
    }

    if (path.endsWith(".ico")) {
        return "image/x-icon";
    }

    if (path.endsWith(".txt")) {
        return "text/plain";
    }

    return "application/octet-stream";
}
