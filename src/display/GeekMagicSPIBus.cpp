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

#include "display/GeekMagicSPIBus.h"

/**
 * @brief Construct a new Geek Magic SPI Bus:: Geek Magic SPI Bus object
 *
 * @param dc Data/Command pin
 * @param cs Chip Select pin
 * @param csActiveHigh Whether CS is active high
 * @param defaultSpeed Default SPI speed
 * @param defaultDataMode Default SPI data mode
 */
GeekMagicSPIBus::GeekMagicSPIBus(int8_t dataCmdPin, int8_t csPin, bool csActiveHigh, int32_t defaultSpeed,
                                 int8_t defaultDataMode)
    : _spi(dataCmdPin, GFX_NOT_DEFINED, &SPI, true),
      _cs(csPin),
      _csActiveHigh(csActiveHigh),
      _defaultSpeed(defaultSpeed),
      _defaultDataMode(defaultDataMode) {}

/**
 * @brief Initializes the SPI bus with the specified speed and data mode
 * @param speed SPI speed in Hz
 * @param dataMode SPI data mode
 *
 * @return true if initialization is successful false otherwise
 */
auto GeekMagicSPIBus::begin(int32_t speed, int8_t dataMode) -> bool {
    if (speed == GFX_NOT_DEFINED) {
        speed = _defaultSpeed;
    }
    if (dataMode == GFX_NOT_DEFINED) {
        dataMode = _defaultDataMode;
    }

    if (_cs != GFX_NOT_DEFINED) {
        pinMode((uint8_t)_cs, OUTPUT);
        digitalWrite((uint8_t)_cs, _csActiveHigh ? LOW : HIGH);
    }

    return _spi.begin(speed, dataMode);
}

/**
 * @brief Begins an SPI write transaction
 *
 * @return void
 */
auto GeekMagicSPIBus::beginWrite() -> void {
    if (_cs != GFX_NOT_DEFINED) {
        digitalWrite((uint8_t)_cs, _csActiveHigh ? HIGH : LOW);
    }
    _spi.beginWrite();
}

/**
 * @brief Ends an SPI write transaction
 *
 * @return void
 */
auto GeekMagicSPIBus::endWrite() -> void {
    _spi.endWrite();

    if (LCD_KEEP_CS_ASSERTED) {
        return;
    }

    if (_cs != GFX_NOT_DEFINED) {
        digitalWrite((uint8_t)_cs, _csActiveHigh ? LOW : HIGH);
    }
}
