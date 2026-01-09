#include <ArduinoJson.h>
#include <LittleFS.h>

#include <Logger.h>
#include "config/ConfigManager.h"

ConfigManager::ConfigManager(const char* filename) : filename(filename) {}

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

    ssid = doc["wifi_ssid"].as<const char*>();
    password = doc["wifi_password"].as<const char*>();

    lcd_enable = doc["lcd_enable"] | lcd_enable;
    lcd_w = doc["lcd_w"] | lcd_w;
    lcd_h = doc["lcd_h"] | lcd_h;
    lcd_rotation = doc["lcd_rotation"] | lcd_rotation;
    lcd_mosi_gpio = doc["lcd_mosi_gpio"] | lcd_mosi_gpio;
    lcd_sck_gpio = doc["lcd_sck_gpio"] | lcd_sck_gpio;
    lcd_cs_gpio = doc["lcd_cs_gpio"] | lcd_cs_gpio;
    lcd_dc_gpio = doc["lcd_dc_gpio"] | lcd_dc_gpio;
    lcd_rst_gpio = doc["lcd_rst_gpio"] | lcd_rst_gpio;
    lcd_cs_active_high = doc["lcd_cs_active_high"] | lcd_cs_active_high;
    lcd_dc_cmd_high = doc["lcd_dc_cmd_high"] | lcd_dc_cmd_high;
    lcd_spi_mode = doc["lcd_spi_mode"] | lcd_spi_mode;
    lcd_keep_cs_asserted = doc["lcd_keep_cs_asserted"] | lcd_keep_cs_asserted;
    lcd_spi_hz = doc["lcd_spi_hz"] | lcd_spi_hz;
    lcd_backlight_gpio = doc["lcd_backlight_gpio"] | lcd_backlight_gpio;
    lcd_backlight_active_low = doc["lcd_backlight_active_low"] | lcd_backlight_active_low;

    return true;
}

/**
 * @brief Retrieves the current Wi-Fi SSID
 *
 * @return The SSID as a c style string
 */
auto ConfigManager::getSSID() -> const char* { return ssid.c_str(); }

/**
 * @brief Retrieves the current Wi-Fi password
 *
 * @return The password as a c style string
 */
auto ConfigManager::getPassword() -> const char* { return password.c_str(); }

/**
 * @brief Returns the current status of the LCD enable flag
 *
 * @return true if the LCD is enabled false otherwise
 */
auto ConfigManager::getLCDEnable() -> bool { return lcd_enable; }

/**
 * @brief Retrieves the LCD width
 *
 * @return The width of the LCD in pixels
 */
auto ConfigManager::getLCDWidth() -> int16_t { return lcd_w; }
/**
 * @brief Retrieves the LCD height
 *
 * @return The height of the LCD in pixels
 */
auto ConfigManager::getLCDHeight() -> int16_t { return lcd_h; }
/**
 * @brief Retrieves the LCD rotation setting
 *
 * @return The rotation of the LCD
 */
auto ConfigManager::getLCDRotation() -> uint8_t { return lcd_rotation; }

/**
 * @brief Retrieves the GPIO pin number for LCD MOSI
 *
 * @return The GPIO pin number for LCD MOSI
 */
auto ConfigManager::getLCDMosiGpio() -> int8_t { return lcd_mosi_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD SCK
 *
 * @return The GPIO pin number for LCD SCK
 */
auto ConfigManager::getLCDSckGpio() -> int8_t { return lcd_sck_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD CS
 *
 * @return The GPIO pin number for LCD CS
 */
auto ConfigManager::getLCDCsGpio() -> int8_t { return lcd_cs_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD DC
 *
 * @return The GPIO pin number for LCD DC
 */
auto ConfigManager::getLCDDcGpio() -> int8_t { return lcd_dc_gpio; }

/**
 * @brief Retrieves the GPIO pin number for LCD RST
 *
 * @return The GPIO pin number for LCD RST
 */
auto ConfigManager::getLCDRstGpio() -> int8_t { return lcd_rst_gpio; }

/**
 * @brief Returns whether the LCD CS pin is active high
 *
 * @return true if the LCD CS pin is active high false otherwise
 */
auto ConfigManager::getLCDCsActiveHigh() -> bool { return lcd_cs_active_high; }
/**
 * @brief Returns whether the LCD DC pin is command high
 *
 * @return true if the LCD DC pin is command high false otherwise
 */
auto ConfigManager::getLCDDcCmdHigh() -> bool { return lcd_dc_cmd_high; }

/**
 * @brief Retrieves the LCD SPI mode
 *
 * @return The SPI mode of the LCD
 */
auto ConfigManager::getLCDSpiMode() -> uint8_t { return lcd_spi_mode; }

/**
 * @brief Returns whether the LCD CS pin is kept asserted
 *
 * @return true if the LCD CS pin is kept asserted false otherwise
 */
auto ConfigManager::getLCDKeepCsAsserted() -> bool { return lcd_keep_cs_asserted; }

/**
 * @brief Retrieves the SPI clock frequency for the LCD
 *
 * @return The SPI clock frequency in Hz
 */
auto ConfigManager::getLCDSpiHz() -> uint32_t { return lcd_spi_hz; }

/**
 * @brief Retrieves the GPIO pin number for the LCD backlight
 *
 * @return The GPIO pin number for the LCD backlight
 */
auto ConfigManager::getLCDBacklightGpio() -> int8_t { return lcd_backlight_gpio; }

/**
 * @brief Returns whether the LCD backlight pin is active low
 *
 * @return true if the LCD backlight pin is active low false otherwise
 */
auto ConfigManager::getLCDBacklightActiveLow() -> bool { return lcd_backlight_active_low; }
