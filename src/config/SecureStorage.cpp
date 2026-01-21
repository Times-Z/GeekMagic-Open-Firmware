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

#include <array>
#include <EEPROM.h>
#include <Logger.h>
#include <ESP8266WiFi.h>
#include <user_interface.h>
#include <BearSSLHelpers.h>

#include "config/SecureStorage.h"

static const std::array<uint8_t, 4> NVS_MAGIC = {{'N', 'V', 'S', '1'}};
static constexpr uint8_t LEN_SHIFT = 8;
static constexpr uint8_t LEN_HIGH_IDX = 4;
static constexpr uint8_t LEN_LOW_IDX = 5;
static constexpr uint8_t LEN_MASK = 0xFF;
static constexpr size_t KEY_LEN = 32;

static String kvSalt;

/**
 * @brief Set the public salt used in key derivation
 *
 * @param salt The public salt string
 */
void SecureStorage::setSalt(const String& salt) { kvSalt = salt; }

/**
 * @brief Derive the obfuscation key from device-unique parameters and public salt
 *
 * @param out32 Output buffer for the 32-byte derived key
 *
 * @returns void
 */
static void deriveKey(uint8_t* out32) {
    String mac = WiFi.macAddress();
    uint32_t chip = system_get_chip_id();

    String input = mac + String(chip) + kvSalt;

    br_sha256_context ctx;
    br_sha256_init(&ctx);
    br_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
    br_sha256_out(&ctx, out32);
}

SecureStorage::SecureStorage(size_t eepromSize) : _eepromSize(eepromSize), _doc() {}

/**
 * @brief Initialize the EEPROM-backed NVS and load any existing data
 *
 *
 * @return true on success false on failure
 */
auto SecureStorage::begin() -> bool {
    Logger::info("EEPROM init start", "SecureStorage");

    EEPROM.begin(static_cast<int>(_eepromSize));

    if (!loadToMemory()) {
        Logger::warn("No existing NVS data found, initializing new storage", "SecureStorage");
        _doc.clear();

        if (!flushToEEPROM()) {
            Logger::error("Failed to initialize NVS in EEPROM", "SecureStorage");
            _ready = false;

            return false;
        }
    }

    _ready = true;

    return true;
}

/**
 * @brief Load existing NVS data from EEPROM into the in-memory JSON document
 *
 * The EEPROM layout is:
 *
 * - bytes 0..3: magic ('N','V','S','x'), x represents version
 *
 * - bytes 4..5: payload length (big-endian)
 *
 * - bytes 6..(6+len-1): JSON payload
 *
 * @return true on success false on failure
 */
auto const SecureStorage::loadToMemory() -> bool {
    // 2 bytes length + 4 bytes magic
    const size_t headerSize = 6;

    if (_eepromSize <= headerSize) {
        return false;
    }

    for (size_t i = 0; i < 4; ++i) {
        if (EEPROM.read((int)i) != NVS_MAGIC[i]) {
            Logger::warn("NVS magic not found in EEPROM", "SecureStorage");

            return false;
        }
    }

    uint16_t len = (static_cast<uint16_t>(EEPROM.read(LEN_HIGH_IDX)) << LEN_SHIFT) |
                   static_cast<uint16_t>(EEPROM.read(LEN_LOW_IDX));
    size_t payloadMax = _eepromSize - headerSize;

    if (len == 0 || len > payloadMax) {
        Logger::warn("Invalid NVS length in EEPROM", "SecureStorage");

        return false;
    }

    char* buf = new char[len + 1];
    for (uint16_t i = 0; i < len; ++i) {
        buf[i] = static_cast<char>(EEPROM.read(static_cast<int>(headerSize + i)));
    }

    buf[len] = '\0';

    // De-obfuscate using derived key
    std::array<uint8_t, KEY_LEN> key;
    deriveKey(key.data());
    for (uint16_t i = 0; i < len; ++i) {
        buf[i] ^= key[static_cast<size_t>(i) % KEY_LEN];
    }

    DeserializationError err = deserializeJson(_doc, buf);

    if (err) {
        Logger::warn(String("Failed to parse NVS JSON: " + String(err.c_str())).c_str(), "SecureStorage");
        _doc.clear();

        delete[] buf;

        return false;
    }

    delete[] buf;

    Logger::info("NVS data loaded from EEPROM", "SecureStorage");

    return true;
}

/**
 * @brief Flush the in-memory JSON document to EEPROM
 *
 * @return true on success false on failure
 */
auto const SecureStorage::flushToEEPROM() -> bool {
    const size_t headerSize = 6;
    size_t payloadMax = _eepromSize - headerSize;

    String out;
    out.reserve(static_cast<int>(payloadMax));
    size_t written = serializeJson(_doc, out);

    if (written == 0 || written > payloadMax) {
        Logger::error("Serialized NVS too large for EEPROM", "SecureStorage");

        return false;
    }

    for (size_t i = 0; i < 4; ++i) {
        EEPROM.write(static_cast<int>(i), NVS_MAGIC[i]);
    }

    auto len = static_cast<uint16_t>(written);
    EEPROM.write(LEN_HIGH_IDX, static_cast<uint8_t>((len >> LEN_SHIFT) & LEN_MASK));
    EEPROM.write(LEN_LOW_IDX, static_cast<uint8_t>(len & LEN_MASK));

    // Obfuscate using derived key
    std::array<uint8_t, KEY_LEN> key;
    deriveKey(key.data());
    for (uint16_t i = 0; i < len; ++i) {
        uint8_t obfuscatedString = out.charAt(i) ^ key[static_cast<size_t>(i) % KEY_LEN];
        EEPROM.write(static_cast<int>(headerSize + i), obfuscatedString);
    }

    if (!EEPROM.commit()) {
        Logger::error("EEPROM commit failed", "SecureStorage");

        return false;
    }

    Logger::info(("NVS commit success size " + String(written)).c_str(), "SecureStorage");

    return true;
}

/**
 * @brief Store a key/value pair in the secure NVS
 *
 * @param key The key to set
 * @param value The string value to store
 * @return true on success false otherwise
 */
auto SecureStorage::put(const char* key, const char* value) -> bool {
    if (!_ready) {
        if (!begin()) {
            Logger::error("SecureStorage not initialized", "SecureStorage");
            return false;
        };
    }

    _doc[key] = value;

    return flushToEEPROM();
}

/**
 * @brief Remove a key from the NVS store
 *
 * @param key The key to remove
 *
 * @return true on success false otherwise
 */
auto SecureStorage::remove(const char* key) -> bool {
    if (!_ready) {
        if (!begin()) {
            return false;
        }
    }

    _doc.remove(key);

    return flushToEEPROM();
}

/**
 * @brief Retrieve a string value from NVS
 *
 * @param key The key to read
 * @param defaultValue Value to return if key missing (optional)
 *
 * @return String containing the stored value or default
 */
auto SecureStorage::get(const char* key, const char* defaultValue) -> String {
    if (!_ready) {
        if (!begin()) {
            return defaultValue != nullptr ? String(defaultValue) : String();
        }
    }

    if (_doc[key].isNull()) {
        return defaultValue != nullptr ? String(defaultValue) : String();
    }

    const char* valuePtr = _doc[key];

    return valuePtr != nullptr ? String(valuePtr) : (defaultValue != nullptr ? String(defaultValue) : String());
}
