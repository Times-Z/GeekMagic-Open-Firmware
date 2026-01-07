#include <ArduinoJson.h>
#include <LittleFS.h>

#include "../lib/Logger/Logger.h"  // Maybe use a better path here if possible
#include "config/ConfigManager.h"

ConfigManager::ConfigManager(const char* filename) : filename(filename) {}

/**
 * @brief Loads the configuration from a file stored in SPIFFS
 *
 * @return true if the configuration was successfully loaded and parsed false otherwise
 */
bool ConfigManager::load() {
    if (!LittleFS.begin()) {
        Logger::error("Failed to mount LittleFS");
        return false;
    }

    File file = LittleFS.open(filename.c_str(), "r");
    if (!file) {
        Logger::error("Failed to open config file");
        return false;
    }

    size_t size = file.size();
    if (size == 0) {
        Logger::warn("Config file is empty");
        file.close();
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size + 1]);
    file.readBytes(buf.get(), size);
    buf[size] = '\0';
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
        Logger::error(("Failed to parse config file : " + String(error.c_str())).c_str());
        return false;
    }

    ssid = doc["wifi_ssid"].as<const char*>();
    password = doc["wifi_password"].as<const char*>();

    return true;
}

/**
 * @brief Retrieves the current Wi-Fi SSID
 *
 * @return The SSID as a c style string
 */
const char* ConfigManager::getSSID() { return ssid.c_str(); }

/**
 * @brief Retrieves the current Wi-Fi password
 *
 * @return The password as a c style string
 */
const char* ConfigManager::getPassword() { return password.c_str(); }
