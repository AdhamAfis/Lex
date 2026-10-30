#include "LanguagePlugin.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include "../include/json.hpp"

LanguagePluginManager* LanguagePluginManager::instance = nullptr;

LanguagePluginManager::LanguagePluginManager() {

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

    pluginMap.clear();
    duplicatePlugin = false;

    if (!std::filesystem::exists(pluginsDirectory)) {
        try {
            std::filesystem::create_directory(pluginsDirectory);
        } catch (const std::exception& e) {
            std::cerr << "Error creating plugins directory: " << e.what() << std::endl;
            return;
        }
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::cout << "Found config file: " << filename << std::endl;

                try {
                    std::ifstream input(entry.path());
                    auto data = nlohmann::json::parse(input);
                    if (!data.is_object() || !data.contains("name") || !data["name"].is_string() || data["name"].get<std::string>().empty()) continue;
                    LanguageConfig config = ConfigLoader::loadLanguageFromFile(entry.path().string());
                    std::string name = config.getName();

                    std::string lowerName = name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                    registerPlugin(lowerName, entry.path().string());

                    if (filename.length() > 12 && filename.substr(filename.length() - 12) == "_config.json") {
                        std::string fileBaseName = filename.substr(0, filename.length() - 12);
                        std::transform(fileBaseName.begin(), fileBaseName.end(), fileBaseName.begin(), ::tolower);

                        if (fileBaseName != lowerName) {
                            registerPlugin(fileBaseName, entry.path().string());
                            std::cout << "  - Also registered as: " << fileBaseName << std::endl;
                        }

                        if (fileBaseName == "cpp") {
                            registerPlugin("c++", entry.path().string());
                            std::cout << "  - Also registered as: c++" << std::endl;
                        } else if (fileBaseName == "js") {
                            registerPlugin("javascript", entry.path().string());
                            std::cout << "  - Also registered as: javascript" << std::endl;
                        } else if (fileBaseName == "python") {
                            registerPlugin("py", entry.path().string());
                            std::cout << "  - Also registered as: py" << std::endl;
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
    if (duplicatePlugin) pluginMap.clear();
}

void LanguagePluginManager::registerPlugin(const std::string& name, const std::string& configPath) {

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto [it, inserted] = pluginMap.emplace(lowerName, configPath);
    if (!inserted && it->second != configPath) {
        std::cerr << "Duplicate language plugin name or alias: " << lowerName << std::endl;
        duplicatePlugin = true;
    }
}

LanguageConfig LanguagePluginManager::loadLanguage(const std::string& name) {

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (!hasLanguage(lowerName)) {
        throw std::runtime_error("Language plugin not found: " + name);
    }

    return ConfigLoader::loadLanguageFromFile(pluginMap[lowerName]);
}

std::vector<std::string> LanguagePluginManager::getAvailableLanguages() const {

    std::unordered_set<std::string> languageSet;
    std::vector<std::string> languages;

    const std::unordered_map<std::string, std::string> aliasMap = {
        {"c++", "cpp"},
        {"javascript", "js"},
        {"py", "python"}
    };

    for (const auto& entry : pluginMap) {
        std::string lang = entry.first;

        bool isAlias = false;
        for (const auto& [alias, canonical] : aliasMap) {
            if (lang == alias) {
                isAlias = true;
                break;
            }
        }

        if (!isAlias) {

            languageSet.insert(lang);
        }
    }

    languages.reserve(languageSet.size());
    for (const auto& lang : languageSet) {
        languages.push_back(lang);
    }

    return languages;
}

bool LanguagePluginManager::hasLanguage(const std::string& name) const {

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    return pluginMap.count(lowerName) > 0;
}
