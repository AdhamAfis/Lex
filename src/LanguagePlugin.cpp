#include "LanguagePlugin.h"
#include <iostream>
#include <algorithm>

// Initialize static instance to nullptr
LanguagePluginManager* LanguagePluginManager::instance = nullptr;

LanguagePluginManager::LanguagePluginManager() {
    // Default to a plugins directory in the current working directory
    pluginsDirectory = "plugins";
}

LanguagePluginManager& LanguagePluginManager::getInstance() {
    if (!instance) {
        instance = new LanguagePluginManager();
    }
    return *instance;
}

void LanguagePluginManager::setPluginsDirectory(const std::string& directory) {
    pluginsDirectory = directory;
    scanForPlugins();
}

void LanguagePluginManager::scanForPlugins() {
    // Clear the current plugin map
    pluginMap.clear();
    
    // Create the directory if it doesn't exist
    if (!std::filesystem::exists(pluginsDirectory)) {
        try {
            std::filesystem::create_directory(pluginsDirectory);
        } catch (const std::exception& e) {
            std::cerr << "Error creating plugins directory: " << e.what() << std::endl;
            return;
        }
    }
    
    // Scan the directory for .json files
    try {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::cout << "Found config file: " << filename << std::endl;
                
                // Try to load the language definition to get its name
                try {
                    LanguageConfig config = ConfigLoader::loadLanguageFromFile(entry.path().string());
                    std::string name = config.getName();
                    
                    // Convert to lowercase for case-insensitive matching
                    std::string lowerName = name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                    
                    // Register the plugin by name
                    registerPlugin(lowerName, entry.path().string());
                    
                    // Also register by filename without _config.json
                    if (filename.length() > 12 && filename.substr(filename.length() - 12) == "_config.json") {
                        std::string fileBaseName = filename.substr(0, filename.length() - 12);
                        std::transform(fileBaseName.begin(), fileBaseName.end(), fileBaseName.begin(), ::tolower);
                        
                        // Only register if different from name
                        if (fileBaseName != lowerName) {
                            registerPlugin(fileBaseName, entry.path().string());
                            std::cout << "  - Also registered as: " << fileBaseName << std::endl;
                        }
                        
                        // Register common language aliases
                        if (fileBaseName == "cpp") {
                            registerPlugin("c++", entry.path().string());
                            std::cout << "  - Also registered as: c++" << std::endl;
                        } else if (fileBaseName == "js") {
                            registerPlugin("javascript", entry.path().string());
                            std::cout << "  - Also registered as: javascript" << std::endl;
                        } else if (fileBaseName == "py") {
                            registerPlugin("python", entry.path().string());
                            std::cout << "  - Also registered as: python" << std::endl;
                        }
                    }
                    
                    std::cout << "Found language plugin: " << name << " (" << filename << ")" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error loading language plugin " << filename << ": " << e.what() << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning plugins directory: " << e.what() << std::endl;
    }
}

void LanguagePluginManager::registerPlugin(const std::string& name, const std::string& configPath) {
    // Register the plugin with a lowercase name for case-insensitive matching
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    pluginMap[lowerName] = configPath;
}

LanguageConfig LanguagePluginManager::loadLanguage(const std::string& name) {
    // Convert name to lowercase for case-insensitive matching
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // Check if the language exists
    if (!hasLanguage(lowerName)) {
        throw std::runtime_error("Language plugin not found: " + name);
    }
    
    // Load the configuration from the file
    return ConfigLoader::loadLanguageFromFile(pluginMap[lowerName]);
}

std::vector<std::string> LanguagePluginManager::getAvailableLanguages() const {
    std::vector<std::string> languages;
    languages.reserve(pluginMap.size());
    
    for (const auto& entry : pluginMap) {
        languages.push_back(entry.first);
    }
    
    return languages;
}

bool LanguagePluginManager::hasLanguage(const std::string& name) const {
    // Convert name to lowercase for case-insensitive matching
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return pluginMap.count(lowerName) > 0;
} 