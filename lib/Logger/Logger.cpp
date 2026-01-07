#include <ctime>
#include "Logger.h"

/**
 * @brief Logs a message with a specified log level
 *
 * @param level The severity level of the log message
 * @param message The message to be logged
 */
void Logger::log(LogLevel level, const char* message) {
    printTime();
    Serial.print("(");
    Serial.print(levelToString(level));
    Serial.print(") ");
    Serial.println(message);
}

/**
 * @brief Logs a debug message
 *
 * @param message The debug message to log
 */
void Logger::debug(const char* message) { log(LOG_DEBUG, message); }

/**
 * @brief Logs an info message
 *
 * @param message The info message to log
 */
void Logger::info(const char* message) { log(LOG_INFO, message); }

/**
 * @brief Logs a warning message
 *
 * @param message The warning message to log
 */
void Logger::warn(const char* message) { log(LOG_WARN, message); }

/**
 * @brief Logs an error message
 *
 * @param message The error message to log
 */
void Logger::error(const char* message) { log(LOG_ERROR, message); }

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
