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

ConfigManager configManager;
const char* AP_SSID = "GeekMagic";
const char* AP_PASSWORD = "$str0ngPa$$w0rd";
WiFiManager* wifiManager = nullptr;
ESP8266HTTPUpdateServer httpUpdater;
static String KV_SALT = "GeekMagicOpenFirmwareIsAwesome";

static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
static constexpr uint32_t BOOT_DELAY_MS = 200;
static constexpr int LOADING_BAR_TEXT_X = 50;
static constexpr int LOADING_BAR_TEXT_Y = 80;
static constexpr int LOADING_BAR_Y = 110;
static constexpr int LOADING_DELAY_MS = 1000;

Webserver* webserver = nullptr;

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
}
