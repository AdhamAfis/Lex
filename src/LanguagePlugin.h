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

class LanguagePluginManager {
private:

    std::map<std::string, std::string> pluginMap;
    bool duplicatePlugin = false;

    std::filesystem::path pluginsDirectory;

    static LanguagePluginManager* instance;

    LanguagePluginManager();

public:

    static LanguagePluginManager& getInstance();

    void setPluginsDirectory(const std::string& directory);

    void scanForPlugins();

    void registerPlugin(const std::string& name, const std::string& configPath);

    LanguageConfig loadLanguage(const std::string& name);

    std::vector<std::string> getAvailableLanguages() const;

    bool hasLanguage(const std::string& name) const;
};

#endif
