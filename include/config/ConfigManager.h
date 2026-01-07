#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <ArduinoJson.h>
#include <string>

class ConfigManager {
   public:
    ConfigManager(const char* filename = "/config.json");
    bool load();
    const char* getSSID();
    const char* getPassword();

   private:
    std::string ssid;
    std::string password;
    std::string filename;
};

#endif  // CONFIG_MANAGER_H
