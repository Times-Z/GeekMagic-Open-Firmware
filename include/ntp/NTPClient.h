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

#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <Arduino.h>

class NTPClient {
   public:
    NTPClient();
    void begin(uint32_t syncIntervalSeconds = 6 * 3600, uint8_t maxRetries = 3);
    void loop();
    bool syncNow();

    bool lastSyncOk() const;
    time_t lastSyncTime() const;
    String lastStatus() const;

   private:
    uint32_t _syncIntervalSeconds = 6 * 3600;
    uint8_t _maxRetries = 3;
    time_t _lastSync = 0;
    bool _lastOk = false;
    String _lastStatus = "never synced";
    unsigned long _nextSyncAttemptMs = 0;
    void performSync();
};

#endif  // NTP_CLIENT_H
