#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include "LanguageConfig.h"

class ConfigLoader {
public:

    static LanguageConfig loadLanguageFromFile(const std::string& filename);

    static bool saveLanguageToFile(const LanguageConfig& config, const std::string& filename);

    static LanguageConfig loadLanguageFromString(const std::string& jsonString);

    static std::string languageToJsonString(const LanguageConfig& config);

};

#endif
