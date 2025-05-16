#ifndef LANGUAGE_PLUGIN_H
#define LANGUAGE_PLUGIN_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>
#include "LanguageConfig.h"
#include "ConfigLoader.h"

// Class to manage language plugins (custom language configurations)
class LanguagePluginManager {
private:
    // Map of language name to configuration file path
    std::map<std::string, std::string> pluginMap;
    
    // Path to the plugins directory
    std::filesystem::path pluginsDirectory;
    
    // Singleton instance
    static LanguagePluginManager* instance;
    
    // Private constructor for singleton pattern
    LanguagePluginManager();
    
public:
    // Get singleton instance
    static LanguagePluginManager& getInstance();
    
    // Set the plugins directory
    void setPluginsDirectory(const std::string& directory);
    
    // Scan for available plugins in the plugins directory
    void scanForPlugins();
    
    // Register a plugin manually
    void registerPlugin(const std::string& name, const std::string& configPath);
    
    // Load a language configuration by name
    LanguageConfig loadLanguage(const std::string& name);
    
    // Get the list of available language plugins
    std::vector<std::string> getAvailableLanguages() const;
    
    // Check if a language plugin is available
    bool hasLanguage(const std::string& name) const;
};

#endif // LANGUAGE_PLUGIN_H 