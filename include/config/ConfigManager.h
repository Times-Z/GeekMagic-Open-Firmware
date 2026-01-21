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

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <ArduinoJson.h>
#include <string>
#include <cstdint>

// LCD configuration defaults for hellocubic lite
static constexpr bool LCD_ENABLE = true;
static constexpr int16_t LCD_W = 240;
static constexpr int16_t LCD_H = 240;
static constexpr uint8_t LCD_ROTATION = 4;
static constexpr int8_t LCD_MOSI_GPIO = 13;
static constexpr int8_t LCD_SCK_GPIO = 14;
static constexpr int8_t LCD_CS_GPIO = 2;
static constexpr int8_t LCD_DC_GPIO = 0;
static constexpr int8_t LCD_RST_GPIO = 15;
static constexpr bool LCD_CS_ACTIVE_HIGH = true;
static constexpr bool LCD_DC_CMD_HIGH = false;
static constexpr uint8_t LCD_SPI_MODE = 0;
static constexpr uint32_t LCD_SPI_HZ = 40000000;
static constexpr int8_t LCD_BACKLIGHT_GPIO = 5;
static constexpr bool LCD_BACKLIGHT_ACTIVE_LOW = true;

class ConfigManager {
   public:
    ConfigManager(const char* filename = "/config.json");
    bool load();
    bool save();
    void setWiFi(const char* newSsid, const char* newPassword);
    const char* getSSID() const;
    const char* getPassword() const;
    bool getLCDEnable() const;
    int16_t getLCDWidth() const;
    int16_t getLCDHeight() const;
    uint8_t getLCDRotation() const;
    int8_t getLCDMosiGpio() const;
    int8_t getLCDSckGpio() const;
    int8_t getLCDCsGpio() const;
    int8_t getLCDDcGpio() const;
    int8_t getLCDRstGpio() const;
    bool getLCDCsActiveHigh() const;
    bool getLCDDcCmdHigh() const;
    uint8_t getLCDSpiMode() const;
    bool getLCDKeepCsAsserted() const;
    uint32_t getLCDSpiHz() const;
    int8_t getLCDBacklightGpio() const;
    bool getLCDBacklightActiveLow() const;

   public:
    bool getLCDEnableSafe() const { return lcd_enable; }
    int16_t getLCDWidthSafe() const { return (lcd_w > 0) ? lcd_w : LCD_W; }
    int16_t getLCDHeightSafe() const { return (lcd_h > 0) ? lcd_h : LCD_H; }
    uint8_t getLCDRotationSafe() const { return lcd_rotation; }
    int8_t getLCDMosiGpioSafe() const { return (lcd_mosi_gpio >= 0) ? lcd_mosi_gpio : LCD_MOSI_GPIO; }
    int8_t getLCDSckGpioSafe() const { return (lcd_sck_gpio >= 0) ? lcd_sck_gpio : LCD_SCK_GPIO; }
    int8_t getLCDCsGpioSafe() const { return (lcd_cs_gpio >= 0) ? lcd_cs_gpio : LCD_CS_GPIO; }
    int8_t getLCDDcGpioSafe() const { return (lcd_dc_gpio >= 0) ? lcd_dc_gpio : LCD_DC_GPIO; }
    int8_t getLCDRstGpioSafe() const { return (lcd_rst_gpio >= 0) ? lcd_rst_gpio : LCD_RST_GPIO; }
    bool getLCDCsActiveHighSafe() const { return lcd_cs_active_high; }
    bool getLCDDcCmdHighSafe() const { return lcd_dc_cmd_high; }
    uint8_t getLCDSpiModeSafe() const { return lcd_spi_mode; }
    bool getLCDKeepCsAssertedSafe() const { return lcd_keep_cs_asserted; }
    uint32_t getLCDSpiHzSafe() const { return (lcd_spi_hz > 0) ? lcd_spi_hz : LCD_SPI_HZ; }
    int8_t getLCDBacklightGpioSafe() const {
        return (lcd_backlight_gpio >= 0) ? lcd_backlight_gpio : LCD_BACKLIGHT_GPIO;
    }
    bool getLCDBacklightActiveLowSafe() const { return lcd_backlight_active_low; }
    std::string ssid;
    std::string password;
    std::string filename;
    bool lcd_enable = true;
    int16_t lcd_w = 240;
    int16_t lcd_h = 240;
    uint8_t lcd_rotation = 4;
    int8_t lcd_mosi_gpio = 13;
    int8_t lcd_sck_gpio = 14;
    int8_t lcd_cs_gpio = 2;
    int8_t lcd_dc_gpio = 0;
    int8_t lcd_rst_gpio = 15;
    bool lcd_cs_active_high = true;
    bool lcd_dc_cmd_high = false;
    uint8_t lcd_spi_mode = 0;
    bool lcd_keep_cs_asserted = true;
    uint32_t lcd_spi_hz = 40000000;
    int8_t lcd_backlight_gpio = 5;
    bool lcd_backlight_active_low = true;
};

#endif  // CONFIG_MANAGER_H
