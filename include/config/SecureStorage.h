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

#ifndef SECURE_STORAGE_H
#define SECURE_STORAGE_H

#include <Arduino.h>
#include <ArduinoJson.h>

class SecureStorage {
   public:
    SecureStorage(size_t eepromSize = 2048);
    bool begin();
    bool put(const char* key, const char* value);
    bool remove(const char* key);
    String get(const char* key, const char* defaultValue = nullptr);

    // Set the public salt (should be called before begin())
    static void setSalt(const String& salt);

   private:
    size_t _eepromSize;
    bool loadToMemory();
    bool flushToEEPROM();
    JsonDocument _doc;
    bool _ready = false;
};

#endif  // SECURE_STORAGE_H
