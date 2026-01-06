#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* staSsid, const char* staPass, const char* apSsid, const char* apPass)
    : _staSsid(staSsid), _staPass(staPass), _apSsid(apSsid), _apPass(apPass) {}

void WiFiManager::begin() {
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
        Serial.println("WiFi Connected!");
        Serial.print("STA IP: http://");
        Serial.println(WiFi.localIP());
        _apMode = false;
    } else {
        Serial.println("WiFi Fail. Starting AP.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(_apSsid, _apPass);
        Serial.print("AP IP: http://");
        Serial.println(WiFi.softAPIP());
        _apMode = true;
    }
}

bool WiFiManager::isApMode() const { return _apMode; }

IPAddress WiFiManager::getIP() const { return _apMode ? WiFi.softAPIP() : WiFi.localIP(); }
