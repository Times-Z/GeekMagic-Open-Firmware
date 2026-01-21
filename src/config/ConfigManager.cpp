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

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <Logger.h>
#include "config/ConfigManager.h"
#include "config/SecureStorage.h"

ConfigManager::ConfigManager(const char* filename) : filename(filename), secure() {}

/**
 * @brief Loads the configuration from a file stored in SPIFFS
 *
 * @return true if the configuration was successfully loaded and parsed false otherwise
 */
auto ConfigManager::load() -> bool {
    if (!LittleFS.begin()) {
        Logger::error("Failed to mount LittleFS", "ConfigManager");
        return false;
    }

    File file = LittleFS.open(filename.c_str(), "r");
    if (!file) {
        Logger::error("Failed to open config file", "ConfigManager");
        return false;
    }

    size_t size = file.size();
    if (size == 0) {
        Logger::warn("Config file is empty", "ConfigManager");
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
        Logger::error(("Failed to parse config file : " + String(error.c_str())).c_str(), "ConfigManager");
        return false;
    }

    String ssid = doc["wifi_ssid"] | "";
    String password = doc["wifi_password"] | "";

    this->lcd_enable = doc["lcd_enable"] | lcd_enable;
    this->lcd_w = doc["lcd_w"] | lcd_w;
    this->lcd_h = doc["lcd_h"] | lcd_h;
    this->lcd_rotation = doc["lcd_rotation"] | lcd_rotation;
    this->lcd_mosi_gpio = doc["lcd_mosi_gpio"] | lcd_mosi_gpio;
    this->lcd_sck_gpio = doc["lcd_sck_gpio"] | lcd_sck_gpio;
    this->lcd_cs_gpio = doc["lcd_cs_gpio"] | lcd_cs_gpio;
    this->lcd_dc_gpio = doc["lcd_dc_gpio"] | lcd_dc_gpio;
    this->lcd_rst_gpio = doc["lcd_rst_gpio"] | lcd_rst_gpio;
    this->lcd_cs_active_high = doc["lcd_cs_active_high"] | lcd_cs_active_high;
    this->lcd_dc_cmd_high = doc["lcd_dc_cmd_high"] | lcd_dc_cmd_high;
    this->lcd_spi_mode = doc["lcd_spi_mode"] | lcd_spi_mode;
    this->lcd_keep_cs_asserted = doc["lcd_keep_cs_asserted"] | lcd_keep_cs_asserted;
    this->lcd_spi_hz = doc["lcd_spi_hz"] | lcd_spi_hz;
    this->lcd_backlight_gpio = doc["lcd_backlight_gpio"] | lcd_backlight_gpio;
    this->lcd_backlight_active_low = doc["lcd_backlight_active_low"] | lcd_backlight_active_low;

    String nvs_ssid = secure.get("wifi_ssid", "");
    String nvs_password = secure.get("wifi_password", "");

    if ((ssid.length() != 0 && nvs_ssid.length() == 0) || (password.length() != 0 && nvs_password.length() == 0)) {
        secure.put("wifi_ssid", ssid.c_str());
        secure.put("wifi_password", password.c_str());

        this->ssid = secure.get("wifi_ssid").c_str();
        this->password = secure.get("wifi_password").c_str();

        // Ensure we delete the wifi credentials from the json config after migrating
        ConfigManager::save();

        Logger::info("WiFi credentials migrated to SecureStorage", "ConfigManager");
    } else {
        this->ssid = secure.get("wifi_ssid").c_str();
        this->password = secure.get("wifi_password").c_str();
    }

    return true;
}

/**
 * @brief Retrieves the current Wi-Fi SSID
 *
 * @return The SSID as a c style string
 */
auto ConfigManager::getSSID() const -> const char* { return ssid.c_str(); }

/**
 * @brief Retrieves the current Wi-Fi password
 *
 * @return The password as a c style string
 */
auto ConfigManager::getPassword() const -> const char* { return password.c_str(); }

/**
 * @brief Returns the current status of the LCD enable flag
 *
 * @return true if the LCD is enabled false otherwise
 */
auto ConfigManager::getLCDEnable() const -> bool { return lcd_enable; }

/**
 * @brief Retrieves the LCD width
 *
 * @return The width of the LCD in pixels
 */
auto ConfigManager::getLCDWidth() const -> int16_t { return lcd_w; }
/**
 * @brief Retrieves the LCD height
 *
 * @return The height of the LCD in pixels
 */
auto ConfigManager::getLCDHeight() const -> int16_t { return lcd_h; }
/**
 * @brief Retrieves the LCD rotation setting
 *
 * @return The rotation of the LCD
 */
auto ConfigManager::getLCDRotation() const -> uint8_t { return lcd_rotation; }

/**
 * @brief Retrieves the GPIO pin number for LCD MOSI
 *
 * @return The GPIO pin number for LCD MOSI
 */
auto ConfigManager::getLCDMosiGpio() const -> int8_t { return lcd_mosi_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD SCK
 *
 * @return The GPIO pin number for LCD SCK
 */
auto ConfigManager::getLCDSckGpio() const -> int8_t { return lcd_sck_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD CS
 *
 * @return The GPIO pin number for LCD CS
 */
auto ConfigManager::getLCDCsGpio() const -> int8_t { return lcd_cs_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD DC
 *
 * @return The GPIO pin number for LCD DC
 */
auto ConfigManager::getLCDDcGpio() const -> int8_t { return lcd_dc_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD RST
 *
 * @return The GPIO pin number for LCD RST
 */
auto ConfigManager::getLCDRstGpio() const -> int8_t { return lcd_rst_gpio; }

/**
 * @brief Returns whether the LCD CS pin is active high
 *
 * @return true if the LCD CS pin is active high false otherwise
 */
auto ConfigManager::getLCDCsActiveHigh() const -> bool { return lcd_cs_active_high; }
/**
 * @brief Returns whether the LCD DC pin is command high
 *
 * @return true if the LCD DC pin is command high false otherwise
 */
auto ConfigManager::getLCDDcCmdHigh() const -> bool { return lcd_dc_cmd_high; }

/**
 * @brief Retrieves the LCD SPI mode
 *
 * @return The SPI mode of the LCD
 */
auto ConfigManager::getLCDSpiMode() const -> uint8_t { return lcd_spi_mode; }

/**
 * @brief Returns whether the LCD CS pin is kept asserted
 *
 * @return true if the LCD CS pin is kept asserted false otherwise
 */
auto ConfigManager::getLCDKeepCsAsserted() const -> bool { return lcd_keep_cs_asserted; }

/**
 * @brief Retrieves the SPI clock frequency for the LCD
 *
 * @return The SPI clock frequency in Hz
 */
auto ConfigManager::getLCDSpiHz() const -> uint32_t { return lcd_spi_hz; }

/**
 * @brief Retrieves the GPIO pin number for the LCD backlight
 *
 * @return The GPIO pin number for the LCD backlight
 */
auto ConfigManager::getLCDBacklightGpio() const -> int8_t { return lcd_backlight_gpio; }

/**
 * @brief Returns whether the LCD backlight pin is active low
 *
 * @return true if the LCD backlight pin is active low false otherwise
 */
auto ConfigManager::getLCDBacklightActiveLow() const -> bool { return lcd_backlight_active_low; }

/**
 * @brief Set WiFi credentials in memory
 * @param newSsid The SSID
 * @param newPassword The password
 *
 * @return void
 */
auto ConfigManager::setWiFi(const char* newSsid, const char* newPassword) -> void {
    if (newSsid != nullptr) {
        ssid = newSsid;
    }
    if (newPassword != nullptr) {
        password = newPassword;
    }
}

/**
 * @brief Save the current configuration to the file
 *
 * @param clearWifiCreds If true wifi credentials will be cleared from json config
 *
 * @return true if the configuration was successfully saved false otherwise
 */
auto ConfigManager::save() -> bool {
    if (!LittleFS.begin()) {
        Logger::error("Failed to mount LittleFS", "ConfigManager");

        return false;
    }

    File file = LittleFS.open(filename.c_str(), "w");

    if (!file) {
        Logger::error("Failed to open config file for writing", "ConfigManager");

        return false;
    }

    JsonDocument doc;

    secure.put("wifi_ssid", this->getSSID());
    secure.put("wifi_password", this->getPassword());

    doc["lcd_rotation"] = lcd_rotation;

    if (serializeJson(doc, file) == 0) {
        Logger::error("Failed to write config file", "ConfigManager");
        file.close();

        return false;
    }

    file.close();
    Logger::info("Configuration saved", "ConfigManager");

    return true;
}

/**
 * @brief Migrate WiFi credentials to SecureStorage
 *
 * @param ssid The SSID to store
 * @param password The password to store
 * @return true on success false on failure
 */
auto ConfigManager::migrateWiFiToSecureStorage(String ssid, String password) -> bool {
    secure.put("wifi_ssid", ssid.c_str());
    secure.put("wifi_password", password.c_str());

    ConfigManager::save();

    Logger::info("WiFi credentials migrated to SecureStorage", "ConfigManager");

    return true;
}
