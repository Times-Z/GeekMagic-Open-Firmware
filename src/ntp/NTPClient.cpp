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

#include "ntp/NTPClient.h"
#include <ctime>
#include <array>
#include <Logger.h>
#include <wireless/WiFiManager.h>
#include "config/ConfigManager.h"

extern ConfigManager configManager;

static constexpr const char* TAG = "NTPClient";

/**
 * @brief Default NTP server
 */
static constexpr const char* DEFAULT_NTP_SERVER1 = "pool.ntp.org";

/**
 * @brief miliseconds per second
 */
static constexpr unsigned long MILLIS_PER_SECOND = 1000UL;

/**
 * @brief Default NTP timeout in milliseconds
 */
static constexpr unsigned long DEFAULT_NTP_TIMEOUT_MS = 10000UL;

/**
 * @brief Poll delay in milliseconds
 */
static constexpr unsigned long POLL_DELAY_MS = 200UL;

/**
 * @brief Retry base delay in milliseconds
 */
static constexpr unsigned long RETRY_BASE_DELAY_MS = 500UL;

/**
 * @brief Reasonable epoch time to 2020/09/13
 */
static constexpr time_t REASONABLE_EPOCH = 1600000000UL;

/**
 * @brief Status buffer size
 */
static constexpr size_t STATUS_BUFFER_SIZE = 64U;

/**
 * @brief Year base for struct tm
 */
static constexpr int TM_YEAR_BASE = 1900;

NTPClient::NTPClient() = default;

/**
 * @brief Initialize the NTP client
 * @param syncIntervalSeconds Sync interval in seconds (default: 6 hours)
 * @param maxRetries Maximum number of retries on failure (default: 3)
 *
 * @return void
 */
void NTPClient::begin(uint32_t syncIntervalSeconds, uint8_t maxRetries) {
    _syncIntervalSeconds = syncIntervalSeconds;
    _maxRetries = maxRetries;
    _lastStatus = "not started";
    _nextSyncAttemptMs = millis();

    Logger::info("NTP client initialized", TAG);
}

/**
 * @brief Main loop to handle periodic NTP sync
 *
 * @return void
 */
void NTPClient::loop() {
    if (!WiFiManager::isConnected()) {
        _lastStatus = "network unavailable";
        return;
    }

    unsigned long nowMs = millis();
    if (nowMs >= _nextSyncAttemptMs) {
        performSync();
        _nextSyncAttemptMs = nowMs + (_syncIntervalSeconds * MILLIS_PER_SECOND);
    }
}

/**
 * @brief Trigger an immediate NTP sync
 *
 * @return true if sync was successful false otherwise
 */
auto NTPClient::syncNow() -> bool {
    if (!WiFiManager::isConnected()) {
        _lastStatus = "network unavailable";
        _lastOk = false;

        Logger::warn("Manual NTP sync requested but network is unavailable", TAG);

        return false;
    }

    performSync();

    return _lastOk;
}

/**
 * @brief Perform the NTP synchronization
 *
 * @return void
 */
void NTPClient::performSync() {
    Logger::info("Starting NTP sync...", TAG);

    int attempt = 0;
    time_t now = 0;

    while (attempt < _maxRetries) {
        attempt++;
        const char* srv = configManager.getNtpServer();
        const char* server1 = (srv != nullptr && srv[0] != '\0') ? srv : DEFAULT_NTP_SERVER1;
        const char* server2 = (srv != nullptr && srv[0] != '\0') ? DEFAULT_NTP_SERVER1 : nullptr;

        if (server2 != nullptr) {
            configTime(0, 0, server1, server2);
        } else {
            configTime(0, 0, server1);
        }

        unsigned long start = millis();
        const unsigned long timeoutMs = DEFAULT_NTP_TIMEOUT_MS;

        while ((millis() - start) < timeoutMs) {
            now = time(nullptr);

            if (now > REASONABLE_EPOCH) {
                break;
            }

            delay(static_cast<unsigned long>(POLL_DELAY_MS));
        }

        if (now > REASONABLE_EPOCH) {
            _lastSync = now;
            _lastOk = true;
            std::array<char, STATUS_BUFFER_SIZE> buf;
            struct tm* tm_info = localtime(&now);

            snprintf(buf.data(), buf.size(), "Synced: %04d-%02d-%02d %02d:%02d:%02d", tm_info->tm_year + TM_YEAR_BASE,
                     tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            _lastStatus = String(buf.data());

            Logger::info(_lastStatus.c_str(), TAG);

            return;
        }

        Logger::warn(String("NTP sync attempt " + String(attempt) + " failed").c_str(), TAG);

        delay(static_cast<unsigned long>(RETRY_BASE_DELAY_MS) * static_cast<unsigned long>(attempt));
    }

    _lastOk = false;
    _lastStatus = "sync failed";

    Logger::error("NTP sync failed after retries", TAG);
}

/**
 * @brief Check if the last sync was successful
 *
 * @return true if last sync was successful or false otherwise
 */
auto NTPClient::lastSyncOk() const -> bool { return _lastOk; }

/**
 * @brief Get the time of the last successful sync
 *
 * @return time_t of the last sync
 */
auto NTPClient::lastSyncTime() const -> time_t { return _lastSync; }

/**
 * @brief Get the last sync status message
 *
 * @return String containing the last status
 */
auto NTPClient::lastStatus() const -> String { return _lastStatus; }
