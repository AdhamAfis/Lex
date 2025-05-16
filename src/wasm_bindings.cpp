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

// Function to initialize plugin system
void initializePlugins() {
    printf("Initializing plugin system...\n");
    auto& pluginManager = LanguagePluginManager::getInstance();
    
    // Set plugin directory
    pluginManager.setPluginsDirectory("plugins");
    printf("Plugin directory set to plugins\n");
    
    // Try to list files in the plugins directory
    try {
        printf("Contents of plugins directory:\n");
        for (const auto& entry : std::filesystem::directory_iterator("plugins")) {
            printf("  - %s\n", entry.path().string().c_str());
        }
    } catch (const std::exception& e) {
        printf("Error listing plugins directory: %s\n", e.what());
    }
    
    // Scan for plugins
    pluginManager.scanForPlugins();
    
    // Log available plugins
    auto plugins = pluginManager.getAvailableLanguages();
    printf("Available plugins after scanning: %zu\n", plugins.size());
    for (const auto& plugin : plugins) {
        printf("Found plugin: %s\n", plugin.c_str());
    }
    
    // List all registered plugins with file paths
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

// Helper function to escape JSON strings
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

// Storage for custom runtime-registered languages
std::map<std::string, LanguageConfig> customLanguages;

// Exported functions need to use extern "C" to avoid name mangling
extern "C" {

// Initialize function - will be called automatically on module load
EMSCRIPTEN_KEEPALIVE
int initModule() {
    printf("Initializing Lex WebAssembly module...\n");
    initializePlugins();
    printf("Module initialization complete\n");
    return 1;
}

// Function to register a custom language configuration at runtime
EMSCRIPTEN_KEEPALIVE
bool registerLanguageConfig(const char* languageId, const char* configJson) {
    if (!languageId || !configJson) {
        return false;
    }
    
    try {
        // Parse the config from JSON using the ConfigLoader
        LanguageConfig config = ConfigLoader::loadLanguageFromString(configJson);
        
        // Store in our custom languages map
        customLanguages[languageId] = config;
        
        return true;
    } catch (const std::exception& e) {
        // Error parsing the config
        return false;
    }
}

// Function to tokenize a string with a given language
EMSCRIPTEN_KEEPALIVE
char* tokenizeString(const char* sourceCode, const char* languageId) {
    if (!sourceCode || !languageId) {
        return nullptr;
    }
    
    std::string source(sourceCode);
    std::string langId(languageId);
    
    // Create a JSON response
    std::stringstream jsonResponse;
    jsonResponse << "{";
    
    try {
        // Get language configuration from plugins or registered languages
        LanguageConfig config;
        
        // First check custom runtime-registered languages
        auto customLangIt = customLanguages.find(langId);
        if (customLangIt != customLanguages.end()) {
            config = customLangIt->second;
        } else {
            // Debug info about the requested language
            printf("Tokenizing with language: %s\n", langId.c_str());
            
            // Handle some common aliases - we need to match the file names
            std::string langKey = langId;
            
            // Map to actual plugin file names
            // File pattern is {key}_config.json
            if (langId == "c++") langKey = "cpp";
            if (langId == "py") langKey = "python";  
            if (langId == "javascript") langKey = "js";
            
            printf("Mapped to plugin key: %s (looking for %s_config.json)\n", langKey.c_str(), langKey.c_str());
            
            // Check if there's a plugin
            auto& pluginManager = LanguagePluginManager::getInstance();
            if (pluginManager.hasLanguage(langKey)) {
                config = pluginManager.loadLanguage(langKey);
                printf("Successfully loaded plugin for %s\n", langKey.c_str());
            } else {
                // Try with different mappings if first attempt failed
                printf("Plugin %s not found, trying alternatives...\n", langKey.c_str());
                
                // If c++ didn't work, try cpp
                if (langId == "c++") {
                    printf("Trying 'cpp' instead of 'c++'\n");
                    if (pluginManager.hasLanguage("cpp")) {
                        config = pluginManager.loadLanguage("cpp");
                        printf("Successfully loaded plugin for cpp\n");
                    }
                }
                
                // If javascript didn't work, try js
                if (langId == "javascript") {
                    printf("Trying 'js' instead of 'javascript'\n");
                    if (pluginManager.hasLanguage("js")) {
                        config = pluginManager.loadLanguage("js");
                        printf("Successfully loaded plugin for js\n");
                    }
                }
                
                // Default to C if not found
                try {
                    printf("Falling back to 'c' plugin\n");
                    config = pluginManager.loadLanguage("c");
                } catch (...) {
                    printf("No fallback plugin available\n");
                    throw std::runtime_error("No language plugins available");
                }
            }
        }
        
        // Create lexer with configuration
        Lexer lexer(source, config);
        
        // Get tokens
        std::vector<Token> tokens = lexer.tokenize();
        
        // Check for errors
        if (lexer.hasErrors()) {
            jsonResponse << "\"error\": \"" << escapeJsonString(lexer.getErrorReport()) << "\",";
        }
        
        // Add tokens to JSON
        jsonResponse << "\"tokens\": [";
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // Skip EOF token
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
        // Handle exceptions
        jsonResponse.str("");
        jsonResponse << "{\"error\": \"" << escapeJsonString(e.what()) << "\", \"tokens\": []}";
    }
    
    jsonResponse << "}";
    
    // Convert string to C-style string that can be returned to JavaScript
    // Note: This memory will be managed by the Emscripten heap
    std::string jsonStr = jsonResponse.str();
    char* result = (char*)malloc(jsonStr.size() + 1);
    strcpy(result, jsonStr.c_str());
    
    return result;
}

// Function to get available language names
EMSCRIPTEN_KEEPALIVE
char* getLanguageNames() {
    std::stringstream jsonResponse;
    jsonResponse << "[";
    
    // Get all languages from plugins
    auto& pluginManager = LanguagePluginManager::getInstance();
    auto plugins = pluginManager.getAvailableLanguages();
    bool firstItem = true;
    
    printf("getLanguageNames: Found %zu languages\n", plugins.size());
    for (const auto& plugin : plugins) {
        printf("getLanguageNames: Processing plugin '%s'\n", plugin.c_str());
        try {
            // Load the language to get its name
            auto config = pluginManager.loadLanguage(plugin);
            printf("getLanguageNames: Loaded language '%s' (%s)\n", 
                   config.getName().c_str(), plugin.c_str());
            
            if (!firstItem) {
                jsonResponse << ",";
            }
            firstItem = false;
            
            jsonResponse << "{\"id\": \"" << plugin << "\", \"name\": \"" 
                       << escapeJsonString(config.getName()) << "\"}";
        } catch (const std::exception& e) {
            // Skip if we can't load it
            printf("getLanguageNames: Error loading plugin '%s': %s\n", plugin.c_str(), e.what());
            continue;
        } catch (...) {
            printf("getLanguageNames: Unknown error loading plugin '%s'\n", plugin.c_str());
            continue;
        }
    }
    
    // Add custom runtime-registered languages
    for (const auto& [id, config] : customLanguages) {
        jsonResponse << ",{\"id\": \"" << id << "\", \"name\": \"" 
                     << escapeJsonString(config.getName()) << "\"}";
    }
    
    jsonResponse << "]";
    
    // Convert string to C-style string that can be returned to JavaScript
    std::string jsonStr = jsonResponse.str();
    char* result = (char*)malloc(jsonStr.size() + 1);
    strcpy(result, jsonStr.c_str());
    
    return result;
}

} // extern "C" 