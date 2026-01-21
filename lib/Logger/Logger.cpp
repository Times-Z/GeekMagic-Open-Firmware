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

#include <ctime>
#include "Logger.h"

/**
 * @brief Logs a message with a specified log level
 *
 * @param level The severity level of the log message
 * @param message The message to be logged
 * @param className optional class name for context
 */
void Logger::log(LogLevel level, const char* message, const char* className) {
    printTime();
    Serial.print("(");
    Serial.print(levelToString(level));
    Serial.print(")");
    Serial.print("::");

    if (className != nullptr && className[0] != '\0') {
        Serial.print(className);
    } else {
        Serial.print("Global");
    }

    Serial.print(": ");

    Serial.println(message);
}

/**
 * @brief Logs a debug message
 *
 * @param message The debug message to log
 * @param className optional class name for context
 */
void Logger::debug(const char* message, const char* className) { log(LOG_DEBUG, message, className); }

/**
 * @brief Logs an info message
 *
 * @param message The info message to log
 * @param className optional class name for context
 */
void Logger::info(const char* message, const char* className) { log(LOG_INFO, message, className); }

/**
 * @brief Logs a warning message
 *
 * @param message The warning message to log
 * @param className optional class name for context
 */
void Logger::warn(const char* message, const char* className) { log(LOG_WARN, message, className); }

/**
 * @brief Logs an error message
 *
 * @param message The error message to log
 * @param className optional class name for context
 */
void Logger::error(const char* message, const char* className) { log(LOG_ERROR, message, className); }

/**
 * @brief Print the current local time in [HH:MM:SS]
 */
void Logger::printTime() {
    char buffer[20];
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    snprintf(buffer, sizeof(buffer), "[%02d:%02d:%02d]", now->tm_hour, now->tm_min, now->tm_sec);
    Serial.print(buffer);
}

/**
 * @brief Converts a LogLevel enum value to its corresponding string representation
 *
 * @param level The log level
 * @return A constant character pointer to the string representation of the log level
 *         Returns "UNKNOWN" if the log level is not recognized
 */
const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG:
            return "DEBUG";
        case LOG_INFO:
            return "INFO";
        case LOG_WARN:
            return "WARN";
        case LOG_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
