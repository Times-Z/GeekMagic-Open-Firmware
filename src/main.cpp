

#include <Arduino.h>
#include "WiFiManager.h"

const char* DEFAULT_STA_SSID = "";
const char* DEFAULT_STA_PASSWORD = "";
const char* AP_SSID = "HelloCubicLite";
const char* AP_PASSWORD = "$str0ngPa$$w0rd";

WiFiManager wifiManager(DEFAULT_STA_SSID, DEFAULT_STA_PASSWORD, AP_SSID, AP_PASSWORD);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\nHelloCubic Lite Open Firmware");

    wifiManager.begin();
}

void loop() {
    // put your main code here, to run repeatedly:
}
