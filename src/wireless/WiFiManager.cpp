#include "wireless/WiFiManager.h"

/**
 * @brief WifiManager constructor
 *
 * @param staSsid The SSID for the WiFi station mode
 * @param staPass The password for the WiFi station mode
 * @param apSsid The SSID for the WiFi access point mode
 * @param apPass The password for the WiFi access point mode
 */
WiFiManager::WiFiManager(const char* staSsid, const char* staPass, const char* apSsid, const char* apPass)
    : _staSsid(staSsid), _staPass(staPass), _apSsid(apSsid), _apPass(apPass) {}

void WiFiManager::begin() {
    if (!WiFiManager::startStationMode()) {
        WiFiManager::startAccessPointMode();
    }

    Serial.println("Wifi active");
    Serial.println("Mode : " + String(_apMode ? "AP" : "STA"));
    Serial.println("SSID : " + String(_apMode ? _apSsid : _staSsid));
    Serial.println("IP   : " + WiFiManager::getIP().toString());
}

/**
 * @brief Attempts to connect the device to a WiFi network in station mode
 *
 * @return true if the device successfully connects to the WiFi network false otherwise
 */
bool WiFiManager::startStationMode() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_staSsid, _staPass);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    Serial.println("");

    if (WiFi.status() == WL_CONNECTED) {
        _apMode = false;

        return true;
    }

    return false;
}

/**
 * @brief Starts the WiFi Access Point (AP) mode
 *
 * @return true Always returns true to indicate the AP mode was started
 */
bool WiFiManager::startAccessPointMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSsid, _apPass);

    _apMode = true;

    return true;
}

bool WiFiManager::isApMode() const { return _apMode; }

IPAddress WiFiManager::getIP() const { return _apMode ? WiFi.softAPIP() : WiFi.localIP(); }
