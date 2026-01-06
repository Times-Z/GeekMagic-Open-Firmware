#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

class WiFiManager {
   public:
    WiFiManager(const char* staSsid, const char* staPass, const char* apSsid, const char* apPass);
    void begin();
    bool isApMode() const;
    IPAddress getIP() const;

   private:
    const char* _staSsid;
    const char* _staPass;
    const char* _apSsid;
    const char* _apPass;
    bool _apMode = false;
};

#endif  // WIFI_MANAGER_H
