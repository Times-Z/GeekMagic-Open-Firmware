#include <Arduino.h>
#include <LittleFS.h>

#include "../lib/Logger/Logger.h"  // Maybe use a better path here if possible
#include "config/ConfigManager.h"
#include "wireless/WiFiManager.h"

ConfigManager configManager;
const char* AP_SSID = "HelloCubicLite";
const char* AP_PASSWORD = "$str0ngPa$$w0rd";
WiFiManager* wifiManager = nullptr;

/**
 * @brief Initializes the system
 *
 */
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("");
    Logger::info("HelloCubic Lite Open Firmware");

    if (!LittleFS.begin()) {
        Logger::error("Failed to mount LittleFS");
        return;
    }

    if (configManager.load()) {
        Logger::info("Configuration loaded successfully");
    }

    wifiManager = new WiFiManager(configManager.getSSID(), configManager.getPassword(), AP_SSID, AP_PASSWORD);
    wifiManager->begin();
}

void loop() {
    // put your main code here, to run repeatedly:
}
