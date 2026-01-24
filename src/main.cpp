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

#include <Arduino.h>
#include <LittleFS.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <ESP8266HTTPUpdateServer.h>

#include <Logger.h>
#include "project_version.h"
#include "config/ConfigManager.h"
#include "wireless/WiFiManager.h"
#include "display/DisplayManager.h"
#include "web/Webserver.h"
#include "web/Api.h"
#include <array>

ConfigManager configManager;
const char* AP_SSID = "GeekMagic";
const char* AP_PASSWORD = "$str0ngPa$$w0rd";
WiFiManager* wifiManager = nullptr;
ESP8266HTTPUpdateServer httpUpdater;
static String KV_SALT = "GeekMagicOpenFirmwareIsAwesome";
uint32_t initial_free_heap = ESP.getFreeHeap();  // NOLINT(readability-static-accessed-through-instance)

static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
static constexpr uint32_t BOOT_DELAY_MS = 200;
static constexpr int LOADING_BAR_TEXT_X = 50;
static constexpr int LOADING_BAR_TEXT_Y = 80;
static constexpr int LOADING_BAR_Y = 110;
static constexpr int LOADING_DELAY_MS = 1000;

Webserver* webserver = nullptr;

/**
 * @brief Formats bytes into a human-readable string
 *
 * @param value Size in bytes
 * @return Formatted string
 */
static auto formatBytes(size_t value) -> String {
    constexpr std::array<const char*, 5> UNITS = {"B", "KB", "MB", "GB", "TB"};
    auto UNITS_COUNT = static_cast<int>(UNITS.size());
    constexpr double THRESHOLD = 1024.0;
    constexpr int INITIAL_UNIT = 0;
    constexpr size_t BUF_SIZE = 32;
    constexpr const char* FRACTIONAL_FORMAT = "%.1f %s";
    constexpr const char* INTEGER_FORMAT = "%u %s";

    auto val = static_cast<double>(value);
    int unit = INITIAL_UNIT;
    while (val >= THRESHOLD && unit < UNITS_COUNT - 1) {
        val /= THRESHOLD;
        ++unit;
    }

    std::array<char, BUF_SIZE> buf{};
    if (unit == INITIAL_UNIT) {
        snprintf(buf.data(), buf.size(), INTEGER_FORMAT, static_cast<unsigned int>(value), UNITS[unit]);
    } else {
        snprintf(buf.data(), buf.size(), FRACTIONAL_FORMAT, val, UNITS[unit]);
    }

    return {buf.data()};
}

/**
 * @brief Initializes the system
 *
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(BOOT_DELAY_MS);
    Serial.println("");
    Logger::info(("GeekMagic Open Firmware " + String(PROJECT_VER_STR)).c_str());

    constexpr int TOTAL_STEPS = 6;
    int step = 0;

    if (!LittleFS.begin()) {
        if (DisplayManager::isReady()) {
            DisplayManager::drawLoadingBar((float)step / TOTAL_STEPS, LOADING_BAR_Y);
        }
        Logger::error("Failed to mount LittleFS");
        return;
    }

    step++;

    SecureStorage::setSalt(KV_SALT);

    if (configManager.secure.begin()) {
        Logger::info("SecureStorage initialized successfully", "ConfigManager");
    }

    step++;

    if (configManager.load()) {
        Logger::info("Configuration loaded successfully");
    }
    step++;

    DisplayManager::begin();
    if (DisplayManager::isReady()) {
        DisplayManager::drawTextWrapped(LOADING_BAR_TEXT_X, LOADING_BAR_TEXT_Y, "Starting...", 2, LCD_WHITE, LCD_BLACK,
                                        true);
        DisplayManager::drawLoadingBar((float)step / TOTAL_STEPS, LOADING_BAR_Y);
    }
    step++;

    wifiManager = new WiFiManager(configManager.getSSID(), configManager.getPassword(), AP_SSID, AP_PASSWORD);
    wifiManager->begin();
    if (DisplayManager::isReady()) {
        DisplayManager::drawLoadingBar((float)step / TOTAL_STEPS, LOADING_BAR_Y);
    }
    step++;

    webserver = new Webserver();
    webserver->begin();
    if (DisplayManager::isReady()) {
        DisplayManager::drawLoadingBar((float)step / TOTAL_STEPS, LOADING_BAR_Y);
    }
    step++;

    registerApiEndpoints(webserver);

    httpUpdater.setup(&webserver->raw(), "/legacyupdate");

    webserver->serveStatic("/", "/web/index.html", "text/html");
    webserver->serveStatic("/header.html", "/web/header.html", "text/html");
    webserver->serveStatic("/footer.html", "/web/footer.html", "text/html");
    webserver->serveStatic("/index.html", "/web/index.html", "text/html");
    webserver->serveStatic("/update.html", "/web/update.html", "text/html");
    webserver->serveStatic("/gif_upload.html", "/web/gif_upload.html", "text/html");
    webserver->serveStatic("/wifi.html", "/web/wifi.html", "text/html");
    webserver->serveStatic("/config.json", "/config.json", "application/json");

    webserver->registerStaticDir("/web/css", "/css", "text/css");
    webserver->registerStaticDir("/web/js", "/js", "application/javascript");

    if (DisplayManager::isReady()) {
        DisplayManager::drawLoadingBar(1.0F, LOADING_BAR_Y);
    }

    delay(LOADING_DELAY_MS);

    DisplayManager::drawStartup(wifiManager->getIP().toString());
}

void loop() {
    if (webserver != nullptr) {
        webserver->handleClient();
    }
    DisplayManager::update();

    static unsigned long last_free_heap_log = 0;
    static constexpr unsigned long FREE_HEAP_LOG_INTERVAL_MS = 10000UL;
    unsigned long now = millis();

    if (now - last_free_heap_log >= FREE_HEAP_LOG_INTERVAL_MS) {
        last_free_heap_log = now;
        String msg = String("Free heap: ") +
                     formatBytes(ESP.getFreeHeap()) +  // NOLINT(readability-static-accessed-through-instance)
                     " (initial: " + formatBytes(initial_free_heap) + ")";
        Logger::info(msg.c_str());
    }
}
