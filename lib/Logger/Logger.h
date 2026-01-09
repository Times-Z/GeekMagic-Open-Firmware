#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR };

class Logger {
   public:
    static void log(LogLevel level, const char* message, const char* className = nullptr);
    static void debug(const char* message, const char* className = nullptr);
    static void info(const char* message, const char* className = nullptr);
    static void warn(const char* message, const char* className = nullptr);
    static void error(const char* message, const char* className = nullptr);

   private:
    static void printTime();
    static const char* levelToString(LogLevel level);
};

#endif  // LOGGER_H
