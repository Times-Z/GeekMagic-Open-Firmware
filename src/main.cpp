#include <Arduino.h>
#include <LittleFS.h>

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
    Serial.println("HelloCubic Lite Open Firmware");

    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return;
    }

    if (configManager.load()) {
        Serial.println("Loaded WiFi config from config.json");
    }

    wifiManager = new WiFiManager(configManager.getSSID(), configManager.getPassword(), AP_SSID, AP_PASSWORD);
    wifiManager->begin();
}

void loop() {
    // put your main code here, to run repeatedly:
}
