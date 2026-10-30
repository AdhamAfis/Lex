#include <emscripten/emscripten.h>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include "Lexer.h"
#include "LanguageConfig.h"
#include "LanguagePlugin.h"
#include "Token.h"
#include "ConfigLoader.h"

void initializePlugins() {
    printf("Initializing plugin system...\n");
    auto& pluginManager = LanguagePluginManager::getInstance();

    pluginManager.setPluginsDirectory("plugins");
    printf("Plugin directory set to plugins\n");

    try {
        printf("Contents of plugins directory:\n");
        for (const auto& entry : std::filesystem::directory_iterator("plugins")) {
            printf("  - %s\n", entry.path().string().c_str());
        }
    } catch (const std::exception& e) {
        printf("Error listing plugins directory: %s\n", e.what());
    }

    pluginManager.scanForPlugins();

    auto plugins = pluginManager.getAvailableLanguages();
    printf("Available plugins after scanning: %zu\n", plugins.size());
    for (const auto& plugin : plugins) {
        printf("Found plugin: %s\n", plugin.c_str());
    }

    printf("Registered plugins with file paths:\n");
    try {
        for (const auto& plugin : plugins) {
            try {
                auto config = pluginManager.loadLanguage(plugin);
                printf("  - %s: %s (%s)\n", plugin.c_str(), config.getName().c_str(), config.getVersion().c_str());
            } catch (const std::exception& e) {
                printf("  - %s: ERROR loading (%s)\n", plugin.c_str(), e.what());
            }
        }
    } catch (const std::exception& e) {
        printf("Error testing plugins: %s\n", e.what());
    }
}

std::string escapeJsonString(const std::string& input) {
    std::string output;
    output.reserve(input.length());

    for (char c : input) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    output += buf;
                } else {
                    output += c;
                }
        }
    }

    return output;
}

std::map<std::string, LanguageConfig> customLanguages;

extern "C" {

EMSCRIPTEN_KEEPALIVE
int initModule() {
    printf("Initializing Lex WebAssembly module...\n");
    initializePlugins();
    printf("Module initialization complete\n");
    return 1;
}

EMSCRIPTEN_KEEPALIVE
bool registerLanguageConfig(const char* languageId, const char* configJson) {
    if (!languageId || !configJson) {
        return false;
    }

    try {

        LanguageConfig config = ConfigLoader::loadLanguageFromString(configJson);

        customLanguages[languageId] = config;

        return true;
    } catch (const std::exception& e) {

        return false;
    }
}

EMSCRIPTEN_KEEPALIVE
char* tokenizeString(const char* sourceCode, const char* languageId) {
    if (!sourceCode || !languageId) {
        return nullptr;
    }

    std::string source(sourceCode);
    std::string langId(languageId);

    std::stringstream jsonResponse;
    jsonResponse << "{";

    try {

        LanguageConfig config;

        auto customLangIt = customLanguages.find(langId);
        if (customLangIt != customLanguages.end()) {
            config = customLangIt->second;
        } else {

            printf("Tokenizing with language: %s\n", langId.c_str());

            std::string langKey = langId;

            if (langId == "c++") langKey = "cpp";
            if (langId == "py") langKey = "python";
            if (langId == "javascript") langKey = "js";

            printf("Mapped to plugin key: %s (looking for %s_config.json)\n", langKey.c_str(), langKey.c_str());

            auto& pluginManager = LanguagePluginManager::getInstance();
            if (pluginManager.hasLanguage(langKey)) {
                config = pluginManager.loadLanguage(langKey);
                printf("Successfully loaded plugin for %s\n", langKey.c_str());
            } else {

                printf("Plugin %s not found, trying alternatives...\n", langKey.c_str());

                if (langId == "c++") {
                    printf("Trying 'cpp' instead of 'c++'\n");
                    if (pluginManager.hasLanguage("cpp")) {
                        config = pluginManager.loadLanguage("cpp");
                        printf("Successfully loaded plugin for cpp\n");
                    }
                }

                if (langId == "javascript") {
                    printf("Trying 'js' instead of 'javascript'\n");
                    if (pluginManager.hasLanguage("js")) {
                        config = pluginManager.loadLanguage("js");
                        printf("Successfully loaded plugin for js\n");
                    }
                }

                try {
                    printf("Falling back to 'c' plugin\n");
                    config = pluginManager.loadLanguage("c");
                } catch (...) {
                    printf("No fallback plugin available\n");
                    throw std::runtime_error("No language plugins available");
                }
            }
        }

        Lexer lexer(source, config);

        std::vector<Token> tokens = lexer.tokenize();

        if (lexer.hasErrors()) {
            jsonResponse << "\"error\": \"" << escapeJsonString(lexer.getErrorReport()) << "\",";
        }

        jsonResponse << "\"tokens\": [";

        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];

            if (token.type == TokenType::EOF_TOKEN && i == tokens.size() - 1) {
                continue;
            }

            jsonResponse << "{";
            jsonResponse << "\"type\": \"" << token.typeToString() << "\",";
            jsonResponse << "\"lexeme\": \"" << escapeJsonString(token.lexeme) << "\",";
            jsonResponse << "\"line\": " << token.location.line << ",";
            jsonResponse << "\"column\": " << token.location.column;
            jsonResponse << "}";

            if (i < tokens.size() - 1 && tokens[i+1].type != TokenType::EOF_TOKEN) {
                jsonResponse << ",";
            }
        }

        jsonResponse << "]";

    } catch (const std::exception& e) {

        jsonResponse.str("");
        jsonResponse << "{\"error\": \"" << escapeJsonString(e.what()) << "\", \"tokens\": []";
    }

    jsonResponse << "}";

    std::string jsonStr = jsonResponse.str();
    char* result = (char*)malloc(jsonStr.size() + 1);
    if (!result) return nullptr;
    strcpy(result, jsonStr.c_str());

    return result;
}

EMSCRIPTEN_KEEPALIVE
char* getLanguageNames() {
    std::stringstream jsonResponse;
    jsonResponse << "[";

    auto& pluginManager = LanguagePluginManager::getInstance();
    auto plugins = pluginManager.getAvailableLanguages();
    bool firstItem = true;

    printf("getLanguageNames: Found %zu languages\n", plugins.size());
    for (const auto& plugin : plugins) {
        printf("getLanguageNames: Processing plugin '%s'\n", plugin.c_str());
        try {

            auto config = pluginManager.loadLanguage(plugin);
            printf("getLanguageNames: Loaded language '%s' (%s)\n",
                   config.getName().c_str(), plugin.c_str());

            if (!firstItem) {
                jsonResponse << ",";
            }
            firstItem = false;

            jsonResponse << "{\"id\": \"" << escapeJsonString(plugin) << "\", \"name\": \""
                       << escapeJsonString(config.getName()) << "\"}";
        } catch (const std::exception& e) {

            printf("getLanguageNames: Error loading plugin '%s': %s\n", plugin.c_str(), e.what());
            continue;
        } catch (...) {
            printf("getLanguageNames: Unknown error loading plugin '%s'\n", plugin.c_str());
            continue;
        }
    }

    for (const auto& [id, config] : customLanguages) {
        if (!firstItem) jsonResponse << ",";
        firstItem = false;
        jsonResponse << "{\"id\": \"" << escapeJsonString(id) << "\", \"name\": \""
                     << escapeJsonString(config.getName()) << "\"}";
    }

    jsonResponse << "]";

    std::string jsonStr = jsonResponse.str();
    char* result = (char*)malloc(jsonStr.size() + 1);
    if (!result) return nullptr;
    strcpy(result, jsonStr.c_str());

    return result;
}

}
