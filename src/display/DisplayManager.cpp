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

#include <SPI.h>
#include <Logger.h>
#include <array>
#include <algorithm>

#include "project_version.h"
#include "display/DisplayManager.h"
#include "display/GeekMagicSPIBus.h"
#include "config/ConfigManager.h"
#include "display/Gif.h"

static Gif s_gif;

extern ConfigManager configManager;

static Arduino_DataBus* g_lcdBus = nullptr;
static Arduino_GFX* g_lcd = nullptr;
static bool g_lcdReady = false;
static bool g_lcdInitializing = false;
static uint32_t g_lcdInitAttempts = 0;
static uint32_t g_lcdInitLastMs = 0;
static bool g_lcdInitOk = false;
static constexpr uint32_t LCD_HARDWARE_RESET_DELAY_MS = 100;
static constexpr uint32_t LCD_BEGIN_DELAY_MS = 10;
static constexpr int16_t DISPLAY_PADDING = 10;
static constexpr int16_t DISPLAY_INFO_Y = 100;

static constexpr int WRAP_MAX_CHARS = 128;
static constexpr int WRAP_MAX_LINE_SLOTS = 10;

/**
 * @brief Push the current line buffer into the output lines array
 *
 * @param outLines The output lines array
 * @param lineBuf The current line buffer
 * @param lineLen The current line length
 * @param lineCount The current line count
 * @param maxLines The maximum number of lines allowed
 *
 * @return void
 */
static void wrapPushLine(std::array<std::array<char, WRAP_MAX_CHARS>, WRAP_MAX_LINE_SLOTS>& outLines,
                         std::array<char, WRAP_MAX_CHARS>& lineBuf, int& lineLen, int& lineCount, int maxLines) {
    if (lineCount >= maxLines) {
        return;
    }

    lineBuf[lineLen] = '\0';
    strncpy(outLines[lineCount].data(), lineBuf.data(), WRAP_MAX_CHARS - 1);
    outLines[lineCount][WRAP_MAX_CHARS - 1] = '\0';
    ++lineCount;

    lineLen = 0;
    lineBuf[0] = '\0';
}

/**
 * @brief Append a word to the current line buffer, wrapping if necessary
 *
 * @param outLines The output lines array
 * @param lineBuf The current line buffer
 * @param lineLen The current line length
 * @param wordBuf The word buffer to append
 * @param wordLen The word length
 * @param maxCharsPerLine The maximum characters per line
 * @param lineCount The current line count
 * @param maxLines The maximum number of lines allowed
 *
 * @return void
 */
static void wrapAppendWord(std::array<std::array<char, WRAP_MAX_CHARS>, WRAP_MAX_LINE_SLOTS>& outLines,
                           std::array<char, WRAP_MAX_CHARS>& lineBuf, int& lineLen,
                           std::array<char, WRAP_MAX_CHARS>& wordBuf, int& wordLen, int maxCharsPerLine, int& lineCount,
                           int maxLines) {
    if (wordLen == 0) {
        return;
    }

    if (wordLen > maxCharsPerLine) {
        if (lineLen != 0) {
            wrapPushLine(outLines, lineBuf, lineLen, lineCount, maxLines);
            if (lineCount >= maxLines) {
                wordLen = 0;

                return;
            }
        }
        int copyLen = (wordLen > maxCharsPerLine) ? maxCharsPerLine : wordLen;
        memcpy(lineBuf.data(), wordBuf.data(), static_cast<size_t>(copyLen));
        lineLen = copyLen;
        wordLen = 0;
        wordBuf[0] = '\0';

        return;
    }
    if (lineLen == 0) {
        memcpy(lineBuf.data(), wordBuf.data(), static_cast<size_t>(wordLen));
        lineLen = wordLen;
        wordLen = 0;
        wordBuf[0] = '\0';

        return;
    }
    if ((lineLen + 1 + wordLen) <= maxCharsPerLine) {
        lineBuf[lineLen] = ' ';
        memcpy(lineBuf.data() + lineLen + 1, wordBuf.data(), static_cast<size_t>(wordLen));
        lineLen += 1 + wordLen;
        wordLen = 0;
        wordBuf[0] = '\0';

        return;
    }
    wrapPushLine(outLines, lineBuf, lineLen, lineCount, maxLines);
    if (lineCount >= maxLines) {
        wordLen = 0;

        return;
    }

    memcpy(lineBuf.data(), wordBuf.data(), static_cast<size_t>(wordLen));
    lineLen = wordLen;
    wordLen = 0;
    wordBuf[0] = '\0';
}

// Screen cmd
static constexpr uint8_t ST7789_SLEEP_DELAY_MS = 120;
static constexpr uint8_t ST7789_SLEEP_OUT = 0x11;
static constexpr uint8_t ST7789_PORCH = 0xB2;
static constexpr uint8_t ST7789_PORCH_SETTINGS = 0x1F;

static constexpr uint8_t ST7789_TEARING_EFFECT = 0x35;
static constexpr uint8_t ST7789_MEMORY_ACCESS_CONTROL = 0x36;
static constexpr uint8_t ST7789_COLORMODE = 0x3A;
static constexpr uint8_t ST7789_COLORMODE_RGB565 = 0x05;

static constexpr uint8_t ST7789_POWER_B7 = 0xB7;
static constexpr uint8_t ST7789_POWER_BB = 0xBB;
static constexpr uint8_t ST7789_POWER_C0 = 0xC0;
static constexpr uint8_t ST7789_POWER_C2 = 0xC2;
static constexpr uint8_t ST7789_POWER_C3 = 0xC3;
static constexpr uint8_t ST7789_POWER_C4 = 0xC4;
static constexpr uint8_t ST7789_POWER_C6 = 0xC6;
static constexpr uint8_t ST7789_POWER_D0 = 0xD0;
static constexpr uint8_t ST7789_POWER_D6 = 0xD6;

static constexpr uint8_t ST7789_GAMMA_POS = 0xE0;
static constexpr uint8_t ST7789_GAMMA_NEG = 0xE1;
static constexpr uint8_t ST7789_GAMMA_CTRL = 0xE4;

static constexpr uint8_t ST7789_INVERSION_ON = 0x21;
static constexpr uint8_t ST7789_DISPLAY_ON = 0x29;

// Porch parameters used in sequence
static constexpr uint8_t ST7789_PORCH_PARAM_HS = 0x1F;
static constexpr uint8_t ST7789_PORCH_PARAM_VS = 0x1F;
static constexpr uint8_t ST7789_PORCH_PARAM_DUMMY = 0x00;
static constexpr uint8_t ST7789_PORCH_PARAM_HBP = 0x33;
static constexpr uint8_t ST7789_PORCH_PARAM_VBP = 0x33;

// Simple params for commands
static constexpr uint8_t ST7789_TEARING_PARAM_OFF = 0x00;
static constexpr uint8_t ST7789_MADCTL_PARAM_DEFAULT = 0x00;
static constexpr uint8_t ST7789_B7_PARAM_DEFAULT = 0x00;
static constexpr uint8_t ST7789_BB_PARAM_VOLTAGE = 0x36;
static constexpr uint8_t ST7789_C0_PARAM_1 = 0x2C;
static constexpr uint8_t ST7789_C2_PARAM_1 = 0x01;
static constexpr uint8_t ST7789_C3_PARAM_1 = 0x13;
static constexpr uint8_t ST7789_C4_PARAM_1 = 0x20;
static constexpr uint8_t ST7789_C6_PARAM_1 = 0x13;
static constexpr uint8_t ST7789_D6_PARAM_1 = 0xA1;
static constexpr uint8_t ST7789_D0_PARAM_1 = 0xA4;
static constexpr uint8_t ST7789_D0_PARAM_2 = 0xA1;

// Gamma parameter blocks
static constexpr std::array<uint8_t, 14> ST7789_GAMMA_POS_DATA = {0xF0, 0x08, 0x0E, 0x09, 0x08, 0x04, 0x2F,
                                                                  0x33, 0x45, 0x36, 0x13, 0x12, 0x2A, 0x2D};
static constexpr std::array<uint8_t, 14> ST7789_GAMMA_NEG_DATA = {0xF0, 0x0E, 0x12, 0x0C, 0x0A, 0x15, 0x2E,
                                                                  0x32, 0x44, 0x39, 0x17, 0x18, 0x2B, 0x2F};
static constexpr std::array<uint8_t, 3> ST7789_GAMMA_CTRL_DATA = {0x1D, 0x00, 0x00};

// Column/row address parameters
static constexpr uint8_t ST7789_ADDR_START_HIGH = 0x00;
static constexpr uint8_t ST7789_ADDR_START_LOW = 0x00;
static constexpr uint8_t ST7789_ADDR_END_HIGH = 0x00;
static constexpr uint8_t ST7789_ADDR_END_LOW = 0xEF;

/**
 * @brief Get the Arduino_GFX instance used for the LCD
 *
 * @return Pointer to the Arduino_GFX instance
 */
auto DisplayManager::getGfx() -> Arduino_GFX* { return g_lcd; }

/**
 * @brief Turn the LCD backlight on
 *
 * @return void
 */
static inline void lcdBacklightOn() {
    int8_t gpio = configManager.getLCDBacklightGpioSafe();
    if (gpio < 0) {
        Logger::warn("No backlight GPIO defined", "DisplayManager");
        return;
    }

    pinMode((uint8_t)gpio, OUTPUT);
    digitalWrite((uint8_t)gpio, configManager.getLCDBacklightActiveLowSafe() ? LOW : HIGH);
}

/**
 * @brief Write a single command byte to the ST7789 via the data bus
 *
 * @return void
 */
static inline void ST7789_WriteCommand(uint8_t cmd) {
    if (g_lcdBus == nullptr) {
        Logger::error("No data bus for LCD", "DisplayManager");

        return;
    }

    g_lcdBus->writeCommand(cmd);
}

/**
 * @brief Write a single data byte to the ST7789 via the data bus
 *
 * @return void
 */
static inline void ST7789_WriteData(uint8_t data) {
    if (g_lcdBus == nullptr) {
        Logger::error("No data bus for LCD", "DisplayManager");

        return;
    };

    g_lcdBus->write(data);
}

/**
 * @brief Run a vendor-specific initialization sequence for the ST7789 panel
 *
 *  - Sleep out (0x11)
 *
 *  - Porch settings (0xB2)
 *
 *  - Tearing effect on (0x35)
 *
 *  - Memory access control/MADCTL (0x36)
 *
 *  - Color mode to 16-bit RGB565 (0x3A)
 *
 *  - Various power control settings (0xB7, 0xBB, 0xC0-0xC6, 0xD0, 0xD6)
 *
 *  - Gamma correction settings (0xE0, 0xE1, 0xE4)
 *
 *  - Display inversion on (0x21)
 *
 *  - Display on (0x29)
 *
 *  - Full window setup and RAMWR command (0x2A, 0x2B, 0x2C)
 *
 * @return void
 */
static void lcdRunVendorInit() {
    if (g_lcdBus == nullptr) {
        Logger::error("No data bus for LCD", "DisplayManager");

        return;
    };

    g_lcdBus->beginWrite();

    ST7789_WriteCommand(ST7789_SLEEP_OUT);
    delay(ST7789_SLEEP_DELAY_MS);
    yield();

    ST7789_WriteCommand(ST7789_PORCH);
    ST7789_WriteData(ST7789_PORCH_PARAM_HS);
    ST7789_WriteData(ST7789_PORCH_PARAM_VS);
    ST7789_WriteData(ST7789_PORCH_PARAM_DUMMY);
    ST7789_WriteData(ST7789_PORCH_PARAM_HBP);
    ST7789_WriteData(ST7789_PORCH_PARAM_VBP);
    yield();

    ST7789_WriteCommand(ST7789_TEARING_EFFECT);
    ST7789_WriteData(ST7789_TEARING_PARAM_OFF);
    yield();

    ST7789_WriteCommand(ST7789_MEMORY_ACCESS_CONTROL);
    ST7789_WriteData(ST7789_MADCTL_PARAM_DEFAULT);
    yield();

    ST7789_WriteCommand(ST7789_COLORMODE);
    ST7789_WriteData(ST7789_COLORMODE_RGB565);
    yield();

    ST7789_WriteCommand(ST7789_POWER_B7);
    ST7789_WriteData(ST7789_B7_PARAM_DEFAULT);
    yield();

    ST7789_WriteCommand(ST7789_POWER_BB);
    ST7789_WriteData(ST7789_BB_PARAM_VOLTAGE);
    yield();

    ST7789_WriteCommand(ST7789_POWER_C0);
    ST7789_WriteData(ST7789_C0_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_C2);
    ST7789_WriteData(ST7789_C2_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_C3);
    ST7789_WriteData(ST7789_C3_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_C4);
    ST7789_WriteData(ST7789_C4_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_C6);
    ST7789_WriteData(ST7789_C6_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_D6);
    ST7789_WriteData(ST7789_D6_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_POWER_D0);
    ST7789_WriteData(ST7789_D0_PARAM_1);
    ST7789_WriteData(ST7789_D0_PARAM_2);
    yield();

    ST7789_WriteCommand(ST7789_POWER_D6);
    ST7789_WriteData(ST7789_D6_PARAM_1);
    yield();

    ST7789_WriteCommand(ST7789_GAMMA_POS);
    for (uint8_t v : ST7789_GAMMA_POS_DATA) {
        ST7789_WriteData(v);
    }
    yield();

    ST7789_WriteCommand(ST7789_GAMMA_NEG);
    for (uint8_t v : ST7789_GAMMA_NEG_DATA) {
        ST7789_WriteData(v);
    }
    yield();

    ST7789_WriteCommand(ST7789_GAMMA_CTRL);
    for (uint8_t v : ST7789_GAMMA_CTRL_DATA) {
        ST7789_WriteData(v);
    }
    yield();

    ST7789_WriteCommand(ST7789_INVERSION_ON);
    yield();

    ST7789_WriteCommand(ST7789_DISPLAY_ON);
    yield();

    ST7789_WriteCommand(ST7789_CASET);
    ST7789_WriteData(ST7789_ADDR_START_HIGH);
    ST7789_WriteData(ST7789_ADDR_START_LOW);
    ST7789_WriteData(ST7789_ADDR_END_HIGH);
    ST7789_WriteData(ST7789_ADDR_END_LOW);
    yield();

    ST7789_WriteCommand(ST7789_RASET);
    ST7789_WriteData(ST7789_ADDR_START_HIGH);
    ST7789_WriteData(ST7789_ADDR_START_LOW);
    ST7789_WriteData(ST7789_ADDR_END_HIGH);
    ST7789_WriteData(ST7789_ADDR_END_LOW);
    yield();

    ST7789_WriteCommand(ST7789_RAMWR);
    yield();

    g_lcdBus->endWrite();
}

/**
 * @brief Perform a hardware reset of the LCD panel
 *
 * Toggles the RST GPIO if defined, with appropriate delays
 *
 * @return void
 */
static void lcdHardReset() {
    int8_t rst_gpio = configManager.getLCDRstGpioSafe();
    if (rst_gpio < 0) {
        Logger::warn("No reset GPIO defined", "DisplayManager");
        return;
    }

    pinMode((uint8_t)rst_gpio, OUTPUT);
    digitalWrite((uint8_t)rst_gpio, HIGH);
    delay(LCD_HARDWARE_RESET_DELAY_MS);
    digitalWrite((uint8_t)rst_gpio, LOW);
    delay(LCD_HARDWARE_RESET_DELAY_MS);
    digitalWrite((uint8_t)rst_gpio, HIGH);
    delay(LCD_HARDWARE_RESET_DELAY_MS);
}

/**
 * @brief Ensure the LCD is initialized and ready for drawing
 *
 * @return void
 */
static void lcdEnsureInit() {
    if (!configManager.getLCDEnableSafe() || g_lcdReady || g_lcdInitializing) {
        return;
    };

    g_lcdInitializing = true;
    g_lcdInitAttempts++;
    g_lcdInitLastMs = millis();
    g_lcdInitOk = false;

    Logger::info("Initialization started", "DisplayManager");

    lcdBacklightOn();
    lcdHardReset();

    if (g_lcd != nullptr) {
        delete static_cast<Arduino_ST7789*>(g_lcd);
        g_lcd = nullptr;
    }
    if (g_lcdBus != nullptr) {
        delete static_cast<GeekMagicSPIBus*>(g_lcdBus);
        g_lcdBus = nullptr;
    }

    SPI.begin();

    int8_t dc_gpio = configManager.getLCDDcGpioSafe();
    int8_t cs_gpio = configManager.getLCDCsGpioSafe();
    bool cs_active_high = configManager.getLCDCsActiveHighSafe();
    uint32_t spi_hz = configManager.getLCDSpiHzSafe();
    uint8_t spi_mode = configManager.getLCDSpiModeSafe();
    uint8_t rotation = configManager.getLCDRotationSafe();
    int16_t lcd_w = configManager.getLCDWidthSafe();
    int16_t lcd_h = configManager.getLCDHeightSafe();

    g_lcdBus = new GeekMagicSPIBus(dc_gpio, cs_gpio, cs_active_high, (int32_t)spi_hz, (int8_t)spi_mode);
    g_lcd = new Arduino_ST7789(g_lcdBus, -1, rotation, true, lcd_w, lcd_h);

    g_lcdBus->begin((int32_t)spi_hz, (int8_t)spi_mode);

    g_lcd->begin();
    delay(LCD_BEGIN_DELAY_MS);

    lcdHardReset();
    g_lcdBus->begin((int32_t)spi_hz, (int8_t)spi_mode);

    lcdRunVendorInit();

    g_lcd->setRotation(rotation);

    g_lcdReady = true;
    g_lcdInitializing = false;
    g_lcdInitOk = true;

    Logger::info(
        ("Pointers g_lcd=" + String((uintptr_t)g_lcd, HEX) + " g_lcdBus=" + String((uintptr_t)g_lcdBus, HEX)).c_str(),
        "DisplayManager");
    Logger::info(("Width=" + String(g_lcd->width()) + " height=" + String(g_lcd->height())).c_str(), "DisplayManager");

    g_lcd->fillScreen(LCD_BLACK);
    g_lcd->setTextColor(LCD_WHITE, LCD_BLACK);

    Logger::info("Initialization completed", "DisplayManager");
}

/**
 * @brief Wrap text into lines fitting within max characters and lines
 *
 * @param text The input text to wrap
 * @param maxCharsPerLine Maximum characters allowed per line
 * @param maxLines Maximum number of lines allowed
 * @param outLines Output array to hold the wrapped lines
 *
 * @return The number of lines used
 */
static auto lcdWrapTextToBuffer(const String& text, int maxCharsPerLine, int maxLines,
                                std::array<std::array<char, WRAP_MAX_CHARS>, WRAP_MAX_LINE_SLOTS>& outLines) -> int {
    int lineCount = 0;

    for (auto& row : outLines) {
        row[0] = '\0';
    }

    std::array<char, WRAP_MAX_CHARS> lineBuf{};
    std::array<char, WRAP_MAX_CHARS> wordBuf{};
    int lineLen = 0;
    int wordLen = 0;

    for (uint32_t i = 0; i < text.length(); ++i) {
        char chr = text.charAt(i);

        if (chr == '\r') {
            continue;
        }

        if (chr == '\n') {
            wrapAppendWord(outLines, lineBuf, lineLen, wordBuf, wordLen, maxCharsPerLine, lineCount, maxLines);
            wrapPushLine(outLines, lineBuf, lineLen, lineCount, maxLines);

            if (lineCount >= maxLines) {
                break;
            }

            continue;
        }

        if (chr == ' ' || chr == '\t') {
            wrapAppendWord(outLines, lineBuf, lineLen, wordBuf, wordLen, maxCharsPerLine, lineCount, maxLines);

            if (lineCount >= maxLines) {
                break;
            }

            continue;
        }

        if (wordLen + 1 < WRAP_MAX_CHARS) {
            wordBuf[wordLen++] = chr;
            wordBuf[wordLen] = '\0';
        }
    }

    wrapAppendWord(outLines, lineBuf, lineLen, wordBuf, wordLen, maxCharsPerLine, lineCount, maxLines);

    if (lineLen != 0 && lineCount < maxLines) {
        wrapPushLine(outLines, lineBuf, lineLen, lineCount, maxLines);
    }

    if (lineCount == 0) {
        outLines[0][0] = '\0';
        lineCount = 1;
    }

    return lineCount;
}

/**
 * @brief Draw text on the display with simple word-wrapping
 *
 * @param startX Starting X coordinate in pixels
 * @param startY Starting Y coordinate in pixels
 * @param text The text to draw (can contain newlines)
 * @param textSize Font size multiplier (integer)
 * @param fgColor Foreground color (16-bit RGB565)
 * @param bgColor Background color (16-bit RGB565)
 * @param clearBg If true, clears the background rectangle before drawing
 *
 * @return void
 */
static void lcdDrawTextWrapped(int16_t startX, int16_t startY, const String& text, uint8_t textSize, uint16_t fgColor,
                               uint16_t bgColor, bool clearBg) {
    const auto screenW = static_cast<int16_t>(g_lcd->width());
    const auto screenH = static_cast<int16_t>(g_lcd->height());

    if (startX < 0) {
        startX = 0;
    }

    if (startY < 0) {
        startY = 0;
    }

    if (startX >= screenW || startY >= screenH) {
        Logger::warn("Text start position out of bounds", "DisplayManager");

        return;
    }

    const auto charW = static_cast<int16_t>(6 * textSize);
    const auto charH = static_cast<int16_t>(8 * textSize);
    if (charW <= 0 || charH <= 0) {
        Logger::warn("Invalid character dimensions", "DisplayManager");

        return;
    }

    int maxCharsPerLine = (screenW - startX) / charW;
    int maxLines = (screenH - startY) / charH;
    if (maxCharsPerLine <= 0 || maxLines <= 0) {
        Logger::warn("No space for text", "DisplayManager");

        return;
    }

    if (maxLines > WRAP_MAX_LINE_SLOTS) {
        maxLines = WRAP_MAX_LINE_SLOTS;
    }

    std::array<std::array<char, WRAP_MAX_CHARS>, WRAP_MAX_LINE_SLOTS> lines{};
    int lineCount = lcdWrapTextToBuffer(text, maxCharsPerLine, maxLines, lines);

    if (clearBg) {
        const auto heightPixels = static_cast<int16_t>(static_cast<int>(lineCount) * static_cast<int>(charH));
        g_lcd->fillRect(startX, startY, static_cast<int16_t>(screenW - startX), static_cast<int16_t>(heightPixels),
                        bgColor);
    }

    g_lcd->setTextSize(textSize);
    g_lcd->setTextColor(fgColor, bgColor);

    for (int li = 0; li < lineCount; ++li) {
        g_lcd->setCursor(startX, static_cast<int16_t>(startY + li * charH));
        g_lcd->print(lines[li].data());
    }
}

/**
 * @brief Initialize the DisplayManager and LCD
 *
 * Ensures the LCD is initialized and ready for drawing
 *
 * @return void
 */
auto DisplayManager::begin() -> void { lcdEnsureInit(); }

/**
 * @brief Check if the display is ready for drawing
 *
 * @return true if ready false otherwise
 */
auto DisplayManager::isReady() -> bool { return g_lcdReady && g_lcd != nullptr && g_lcdInitOk; }

/**
 * @brief Draw the startup screen on the LCD
 *
 * @return void
 */
auto DisplayManager::drawStartup(String currentIP) -> void {
    if (!DisplayManager::isReady()) {
        Logger::warn("Display not ready", "DisplayManager");

        return;
    }

    int constexpr rgbDelayMs = 1000;

    g_lcd->fillScreen(LCD_RED);
    delay(rgbDelayMs);
    g_lcd->fillScreen(LCD_GREEN);
    delay(rgbDelayMs);
    g_lcd->fillScreen(LCD_BLUE);
    delay(rgbDelayMs);

    g_lcd->fillScreen(LCD_BLACK);

    int constexpr titleY = 10;
    int constexpr fontSize = 2;

    DisplayManager::drawTextWrapped(DISPLAY_PADDING, titleY, "GeekMagic Open Firmware", fontSize, LCD_WHITE, LCD_BLACK,
                                    false);
    DisplayManager::drawTextWrapped(DISPLAY_PADDING, titleY + THREE_LINES_SPACE, String(PROJECT_VER_STR), fontSize,
                                    LCD_WHITE, LCD_BLACK, false);
    DisplayManager::drawTextWrapped(DISPLAY_PADDING, (titleY + THREE_LINES_SPACE + TWO_LINES_SPACE), "IP: " + currentIP,
                                    fontSize, LCD_WHITE, LCD_BLACK, false);

    const int16_t box = 40;
    const int16_t gap = 20;
    const int16_t boxY = titleY + (THREE_LINES_SPACE * 2) + ONE_LINE_SPACE;

    g_lcd->fillRect(DISPLAY_PADDING, boxY, box, box, LCD_RED);
    g_lcd->fillRect((int16_t)(DISPLAY_PADDING + box + gap), boxY, box, box, LCD_GREEN);
    g_lcd->fillRect((int16_t)(DISPLAY_PADDING + (box + gap) * 2), boxY, box, box, LCD_BLUE);

    yield();

    Logger::info("Startup screen drawn", "DisplayManager");
}

/**
 * @brief Draw text on the display with simple word-wrapping
 *
 * @param x Starting X coordinate in pixels
 * @param y Starting Y coordinate in pixels
 * @param text The text to draw (can contain newlines)
 * @param textSize Font size multiplier (integer)
 * @param fg Foreground color (16-bit RGB565)
 * @param bg Background color (16-bit RGB565)
 * @param clearBg If true, clears the background rectangle before drawing
 *
 * @return void
 */
void DisplayManager::drawTextWrapped(int16_t xPos, int16_t yPos, const String& text, uint8_t textSize, uint16_t fgColor,
                                     uint16_t bgColor, bool clearBg) {
    lcdDrawTextWrapped(xPos, yPos, text, textSize, fgColor, bgColor, clearBg);
}

/**
 * @brief Draw a loading bar on the display
 *
 * @param progress Progress value between 0.0 (empty) and 1.0 (full)
 * @param yPos Y coordinate of the top of the loading bar
 * @param barWidth Width of the loading bar in pixels
 * @param barHeight Height of the loading bar in pixels
 * @param fgColor Foreground color (16-bit RGB565)
 * @param bgColor Background color (16-bit RGB565)
 */
void DisplayManager::drawLoadingBar(float progress, int yPos, int barWidth, int barHeight, uint16_t fgColor,
                                    uint16_t bgColor) {
    if ((g_lcd == nullptr) || (!g_lcdReady)) {
        return;
    }

    auto barXPos = (static_cast<int32_t>(configManager.getLCDWidthSafe()) - static_cast<int32_t>(barWidth)) / 2;
    auto barXPos16 = static_cast<int16_t>(barXPos);
    auto yPos16 = static_cast<int16_t>(yPos);
    auto barWidth16 = static_cast<int16_t>(barWidth);
    auto barHeight16 = static_cast<int16_t>(barHeight);

    g_lcd->fillRect(barXPos16, yPos16, barWidth16, barHeight16, bgColor);

    auto fillWidthF = static_cast<float>(barWidth) * progress;
    auto fillWidth16 = static_cast<int16_t>(fillWidthF);
    if (fillWidth16 > 0) {
        g_lcd->fillRect(barXPos16, yPos16, fillWidth16, barHeight16, fgColor);
    }

    yield();
}

/**
 * @brief Play a single GIF file in full screen mode (blocking)
 *
 * @param path Path to the GIF file on LittleFS
 * @param timeMs Duration to play the GIF in milliseconds (0 = play full GIF)
 * @return true if played successfully, false on error
 */
auto DisplayManager::playGifFullScreen(const String& path, uint32_t timeMs) -> bool {
    // Ensure any currently playing GIF is stopped so we can start a new one
    s_gif.stop();

    if (!s_gif.begin()) {
        return false;
    }

    DisplayManager::clearScreen();

    s_gif.setLoopEnabled(timeMs == 0);

    const bool started = s_gif.playOne(path);
    if (!started) {
        return false;
    }

    if (timeMs == 0) {
        return true;
    }

    const uint32_t startMs = millis();
    const uint32_t endMs = startMs + timeMs;

    while (s_gif.isPlaying() && static_cast<int32_t>(millis() - endMs) < 0) {
        s_gif.update();
        yield();
    }

    if (s_gif.isPlaying()) {
        s_gif.stop();
    }

    while (s_gif.isPlaying()) {
        s_gif.update();
        yield();
    }

    s_gif.setLoopEnabled(false);

    return true;
}

/**
 * @brief Stop GIF playback if playing
 *
 * @return true
 */
auto DisplayManager::stopGif() -> bool {
    s_gif.stop();

    DisplayManager::clearScreen();

    return true;
}

auto DisplayManager::update() -> void { s_gif.update(); }

/**
 * @brief Clear the entire display to black
 *
 * @return void
 */
auto DisplayManager::clearScreen() -> void {
    if (g_lcdReady && g_lcd != nullptr) {
        g_lcd->fillScreen(LCD_BLACK);
    }
}
