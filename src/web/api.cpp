#include <Arduino.h>
#include <Logger.h>
#include <ArduinoJson.h>

#include "web/Webserver.h"

/**
 * @brief Register API endpoints for the webserver
 * @param webserver Pointer to the Webserver instance
 *
 * @return void
 */
void registerApiEndpoints(Webserver* webserver) {
    Logger::info("Registering API endpoints", "API");

    webserver->on("/api/v1/reboot", HTTP_POST, [webserver]() {
        JsonDocument doc;
        int constexpr rebootDelayMs = 1000;

        doc["status"] = "rebooting";
        String json;
        serializeJson(doc, json);

        webserver->raw().send(HTTP_CODE_OK, "application/json", json);

        delay(rebootDelayMs);
        ESP.restart();  // NOLINT(readability-static-accessed-through-instance)
    });
}
