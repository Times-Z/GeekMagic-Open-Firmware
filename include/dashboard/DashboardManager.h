// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>

class DashboardManager {
   public:
    static void begin(const char* metricsUrl);
    static void update();
};
