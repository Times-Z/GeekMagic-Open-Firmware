// SPDX-License-Identifier: GPL-3.0-or-later
#include "dashboard/DashboardManager.h"

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <array>
#include <cstring>

#include "display/DisplayManager.h"

namespace {
constexpr uint32_t FETCH_INTERVAL_MS = 1000;
constexpr uint16_t FETCH_TIMEOUT_MS = 800;
constexpr size_t TILE_COUNT = 6;
constexpr size_t VALUE_TEXT_SIZE = 12;
constexpr uint16_t COLOR_BG = 0x0841;
constexpr uint16_t COLOR_HEADER = 0x18E3;
constexpr uint16_t COLOR_PANEL = 0x2104;
constexpr uint16_t COLOR_BORDER = 0x39E7;
constexpr uint16_t COLOR_MUTED = 0xA514;
constexpr uint16_t COLOR_GPU = 0xF926;
constexpr uint16_t COLOR_CPU = 0xFD20;
constexpr uint16_t COLOR_VRAM = 0xFDE0;
constexpr uint16_t COLOR_RAM = 0xA81F;
constexpr uint16_t COLOR_POWER = 0xFA80;
constexpr uint16_t COLOR_TEMP = 0x3DDF;

String metricsUrl;
uint32_t lastFetch = 0;
bool offlineDrawn = false;
bool dashboardDrawn = false;
std::array<std::array<char, VALUE_TEXT_SIZE>, TILE_COUNT> lastValues{};

struct Metrics {
    float gpuUsage = 0;
    float cpuUsage = 0;
    float gpuVramMb = 0;
    float memoryUsedGb = 0;
    float gpuPower = 0;
    float gpuTemp = 0;
} data;

// The dashboard is a fixed 240x240 pixel layout. Keeping its literal geometry
// together makes the drawing code easier to compare with the physical screen.
// NOLINTBEGIN(readability-magic-numbers,bugprone-narrowing-conversions)
void drawHeader(bool live) {
    auto* gfx = DisplayManager::getGfx();
    gfx->fillRect(0, 0, 240, 26, COLOR_HEADER);
    gfx->setTextColor(LCD_WHITE, COLOR_HEADER);
    gfx->setTextSize(1);
    gfx->setCursor(9, 9);
    gfx->print("SYSTEM METRICS");
    gfx->setTextColor(live ? LCD_GREEN : LCD_RED, COLOR_HEADER);
    gfx->setCursor(207, 9);
    gfx->print(live ? "LIVE" : "OFF");
}

void drawTileFrame(int16_t xPos, int16_t yPos, const char* label, const char* unit, uint16_t accent) {
    auto* gfx = DisplayManager::getGfx();
    constexpr int16_t width = 113;
    constexpr int16_t height = 65;
    gfx->fillRoundRect(xPos, yPos, width, height, 4, COLOR_PANEL);
    gfx->drawRoundRect(xPos, yPos, width, height, 4, COLOR_BORDER);
    gfx->fillRect(xPos + 1, yPos + 1, 4, height - 2, accent);
    gfx->setTextColor(COLOR_MUTED, COLOR_PANEL);
    gfx->setTextSize(1);
    gfx->setCursor(xPos + 12, yPos + 8);
    gfx->print(label);
    gfx->setTextColor(accent, COLOR_PANEL);
    gfx->setCursor(xPos + 88, yPos + 47);
    gfx->print(unit);
}

void resetValueCache() {
    for (auto& value : lastValues) {
        value[0] = '\0';
    }
}

void updateTileValue(uint8_t index, int16_t xPos, int16_t yPos, float value, uint8_t decimals) {
    std::array<char, VALUE_TEXT_SIZE> valueText{};
    dtostrf(value, 0, decimals, valueText.data());
    if (std::strcmp(lastValues[index].data(), valueText.data()) == 0) {
        return;
    }
    auto* gfx = DisplayManager::getGfx();
    gfx->fillRect(xPos + 8, yPos + 25, 78, 35, COLOR_PANEL);
    gfx->setTextColor(LCD_WHITE, COLOR_PANEL);
    gfx->setTextSize(3);
    gfx->setCursor(xPos + 11, yPos + 28);
    gfx->print(valueText.data());
    std::strncpy(lastValues[index].data(), valueText.data(), lastValues[index].size() - 1);
    lastValues[index].back() = '\0';
}

void drawDashboardFrame() {
    auto* gfx = DisplayManager::getGfx();
    gfx->fillScreen(COLOR_BG);
    drawHeader(true);
    drawTileFrame(5, 31, "GPU LOAD", "%", COLOR_GPU);
    drawTileFrame(122, 31, "CPU LOAD", "%", COLOR_CPU);
    drawTileFrame(5, 101, "VRAM USED", "GB", COLOR_VRAM);
    drawTileFrame(122, 101, "RAM USED", "GB", COLOR_RAM);
    drawTileFrame(5, 171, "GPU POWER", "W", COLOR_POWER);
    drawTileFrame(122, 171, "GPU TEMP", "C", COLOR_TEMP);
    resetValueCache();
    dashboardDrawn = true;
}

void updateDashboardValues() {
    updateTileValue(0, 5, 31, data.gpuUsage, 0);
    updateTileValue(1, 122, 31, data.cpuUsage, 0);
    updateTileValue(2, 5, 101, data.gpuVramMb / 1024.0F, 1);
    updateTileValue(3, 122, 101, data.memoryUsedGb, 1);
    updateTileValue(4, 5, 171, data.gpuPower, 0);
    updateTileValue(5, 122, 171, data.gpuTemp, 0);
}

void drawOffline() {
    auto* gfx = DisplayManager::getGfx();
    gfx->fillScreen(COLOR_BG);
    drawHeader(false);
    gfx->setTextColor(LCD_WHITE, COLOR_BG);
    gfx->setTextSize(2);
    gfx->setCursor(49, 91);
    gfx->print("PC OFFLINE");
    gfx->setTextColor(COLOR_MUTED, COLOR_BG);
    gfx->setTextSize(1);
    gfx->setCursor(38, 126);
    gfx->print("Check service or WiFi");
    offlineDrawn = true;
    dashboardDrawn = false;
    resetValueCache();
}
// NOLINTEND(readability-magic-numbers,bugprone-narrowing-conversions)

auto fetchMetrics() -> bool {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(FETCH_TIMEOUT_MS);
    if (!http.begin(client, metricsUrl)) {
        return false;
    }
    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        http.end();
        return false;
    }
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, http.getStream());
    http.end();
    if (error || !doc["ok"].as<bool>()) {
        return false;
    }
    data.gpuUsage = doc["gpu_usage"] | 0.0F;
    data.cpuUsage = doc["cpu_usage"] | 0.0F;
    data.gpuVramMb = doc["gpu_vram_mb"] | 0.0F;
    data.memoryUsedGb = doc["memory_used_gb"] | 0.0F;
    data.gpuPower = doc["gpu_power"] | 0.0F;
    data.gpuTemp = doc["gpu_temp_c"] | 0.0F;
    return true;
}
}  // namespace

void DashboardManager::begin(const char* url) {
    metricsUrl = url;
    lastFetch = millis() - FETCH_INTERVAL_MS;
    drawOffline();
}

void DashboardManager::update() {
    const uint32_t now = millis();
    if (now - lastFetch < FETCH_INTERVAL_MS) {
        return;
    }
    lastFetch = now;
    if (!fetchMetrics()) {
        if (!offlineDrawn) {
            drawOffline();
        }
        return;
    }
    offlineDrawn = false;
    if (!dashboardDrawn) {
        drawDashboardFrame();
    }
    updateDashboardValues();
}
