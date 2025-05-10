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
    // Load language configuration from a JSON file
    static LanguageConfig loadLanguageFromFile(const std::string& filename);
    
    // Save language configuration to a JSON file
    static bool saveLanguageToFile(const LanguageConfig& config, const std::string& filename);
    
    // Load from JSON string
    static LanguageConfig loadLanguageFromString(const std::string& jsonString);
    
    // Convert language config to JSON string
    static std::string languageToJsonString(const LanguageConfig& config);

private:
    // Helper methods to parse JSON sections
    static void parseKeywordSets(LanguageConfig& config, const std::string& jsonString);
    static void parseCharacterSets(LanguageConfig& config, const std::string& jsonString);
    static void parseCommentConfig(LanguageConfig& config, const std::string& jsonString);
    static void parseStringConfig(LanguageConfig& config, const std::string& jsonString);
    static void parseNumberConfig(LanguageConfig& config, const std::string& jsonString);
    static void parsePreprocessorConfig(LanguageConfig& config, const std::string& jsonString);
    static void parseTokenRules(LanguageConfig& config, const std::string& jsonString);
};

#endif // CONFIG_LOADER_H 