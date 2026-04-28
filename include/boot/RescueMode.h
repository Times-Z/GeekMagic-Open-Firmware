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

#ifndef RESCUE_MODE_H
#define RESCUE_MODE_H

#include <cstdint>

// Boot loop detection: if the device reboots BOOT_LOOP_THRESHOLD times
// before BOOT_STABLE_MS elapses, rescue mode is entered
static constexpr uint32_t BOOT_LOOP_THRESHOLD = 3;
static constexpr uint32_t BOOT_STABLE_MS = 20000;

class RescueMode {
   public:
    static bool checkBootLoop();
    static void markBootStable();
    static bool isActive();
    static void run();
    static void loop();

   private:
    static void drawDebugScreen();
    static void registerRescueApi();
    static bool _active;
};

#endif  // RESCUE_MODE_H
