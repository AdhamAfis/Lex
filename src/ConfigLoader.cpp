#include "ConfigLoader.h"
#include <iostream>
#include <regex>
#include "../include/json.hpp"

// Use nlohmann/json library for more robust JSON parsing
using json = nlohmann::json;

// Helper function to unescape regex patterns
std::string unescapeRegexPattern(const std::string& input);

// Simple JSON parser

LanguageConfig ConfigLoader::loadLanguageFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadLanguageFromString(buffer.str());
}

bool ConfigLoader::saveLanguageToFile(const LanguageConfig& config, const std::string& filename) {
    std::string jsonContent = languageToJsonString(config);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << jsonContent;
    return file.good();
}

LanguageConfig ConfigLoader::loadLanguageFromString(const std::string& jsonString) {
    try {
        // Parse the JSON string
        json j = json::parse(jsonString);
        
        // Get basic properties
        std::string name = j.value("name", "Custom");
        std::string version = j.value("version", "1.0");
        
        // Create the configuration
        LanguageConfig config(name, version);
        
        // Keywords
        if (j.contains("keywords") && j["keywords"].is_array()) {
            for (const auto& keyword : j["keywords"]) {
                if (keyword.is_string()) {
                    config.addKeyword(keyword);
                }
            }
        }
        
        // Types
        if (j.contains("types") && j["types"].is_array()) {
            for (const auto& type : j["types"]) {
                if (type.is_string()) {
                    config.addType(type);
                }
            }
        }
        
        // Character sets
        if (j.contains("characterSets") && j["characterSets"].is_object()) {
            const auto& charSets = j["characterSets"];
            
            if (charSets.contains("identifierStart") && charSets["identifierStart"].is_string()) {
                config.characterSets.identifierStart = charSets["identifierStart"];
            }
            
            if (charSets.contains("identifierContinue") && charSets["identifierContinue"].is_string()) {
                config.characterSets.identifierContinue = charSets["identifierContinue"];
            }
            
            if (charSets.contains("operators") && charSets["operators"].is_string()) {
                config.characterSets.operators = charSets["operators"];
            }
            
            if (charSets.contains("delimiters") && charSets["delimiters"].is_string()) {
                config.characterSets.delimiters = charSets["delimiters"];
            }
            
            if (charSets.contains("whitespace") && charSets["whitespace"].is_string()) {
                config.characterSets.whitespace = charSets["whitespace"];
            }
        }
        
        // Comment configuration
        if (j.contains("commentConfig") && j["commentConfig"].is_object()) {
            const auto& commentCfg = j["commentConfig"];
            
            // Single line comment starts
            if (commentCfg.contains("singleLineCommentStarts") && commentCfg["singleLineCommentStarts"].is_array()) {
                for (const auto& start : commentCfg["singleLineCommentStarts"]) {
                    if (start.is_string()) {
                        config.commentConfig.singleLineCommentStarts.push_back(start);
                    }
                }
            }
            
            // Multi-line comment delimiters
            if (commentCfg.contains("multiLineCommentDelimiters") && commentCfg["multiLineCommentDelimiters"].is_array()) {
                for (const auto& delim : commentCfg["multiLineCommentDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.commentConfig.multiLineCommentDelimiters.push_back({start, end});
                    }
                }
            }
            
            // Doc comment starts
            if (commentCfg.contains("docCommentStarts") && commentCfg["docCommentStarts"].is_array()) {
                for (const auto& start : commentCfg["docCommentStarts"]) {
                    if (start.is_string()) {
                        config.commentConfig.docCommentStarts.push_back(start);
                    }
                }
            }
            
            // Doc comment delimiters
            if (commentCfg.contains("docCommentDelimiters") && commentCfg["docCommentDelimiters"].is_array()) {
                for (const auto& delim : commentCfg["docCommentDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.commentConfig.docCommentDelimiters.push_back({start, end});
                    }
                }
            }
        }
        
        // String configuration
        if (j.contains("stringConfig") && j["stringConfig"].is_object()) {
            const auto& stringCfg = j["stringConfig"];
            
            // String delimiters
            if (stringCfg.contains("stringDelimiters") && stringCfg["stringDelimiters"].is_array()) {
                for (const auto& delim : stringCfg["stringDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.stringConfig.stringDelimiters.push_back({start, end});
                    }
                }
            }
            
            // Character delimiters
            if (stringCfg.contains("charDelimiters") && stringCfg["charDelimiters"].is_array()) {
                for (const auto& delim : stringCfg["charDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.stringConfig.charDelimiters.push_back({start, end});
                    }
                }
            }
            
            // Escape character
            if (stringCfg.contains("escapeChar") && stringCfg["escapeChar"].is_string()) {
                std::string escapeStr = stringCfg["escapeChar"];
                if (!escapeStr.empty()) {
                    config.stringConfig.escapeChar = escapeStr[0];
                }
            }
            
            // Raw string prefix
            if (stringCfg.contains("rawStringPrefix") && stringCfg["rawStringPrefix"].is_string()) {
                config.stringConfig.rawStringPrefix = stringCfg["rawStringPrefix"];
            }
        }
        
        // Number configuration
        if (j.contains("numberConfig") && j["numberConfig"].is_object()) {
            const auto& numCfg = j["numberConfig"];
            
            if (numCfg.contains("decimalIntPattern") && numCfg["decimalIntPattern"].is_string()) {
                config.numberConfig.decimalIntPattern = numCfg["decimalIntPattern"];
            }
            
            if (numCfg.contains("floatingPointPattern") && numCfg["floatingPointPattern"].is_string()) {
                config.numberConfig.floatingPointPattern = numCfg["floatingPointPattern"];
            }
            
            if (numCfg.contains("hexPattern") && numCfg["hexPattern"].is_string()) {
                config.numberConfig.hexPattern = numCfg["hexPattern"];
            }
            
            if (numCfg.contains("octalPattern") && numCfg["octalPattern"].is_string()) {
                config.numberConfig.octalPattern = numCfg["octalPattern"];
            }
            
            if (numCfg.contains("binaryPattern") && numCfg["binaryPattern"].is_string()) {
                config.numberConfig.binaryPattern = numCfg["binaryPattern"];
            }
            
            if (numCfg.contains("scientificPattern") && numCfg["scientificPattern"].is_string()) {
                config.numberConfig.scientificPattern = numCfg["scientificPattern"];
            }
        }
        
        // Token rules
        if (j.contains("tokenRules") && j["tokenRules"].is_array()) {
            for (const auto& rule : j["tokenRules"]) {
                if (rule.is_object() && 
                    rule.contains("name") && rule.contains("pattern") && 
                    rule.contains("type") && rule.contains("precedence")) {
                    
                    std::string name = rule["name"];
                    std::string pattern = rule["pattern"];
                    
                    // Unescape the regex pattern before creating the TokenRule
                    pattern = unescapeRegexPattern(pattern);
                    
                    TokenType type = static_cast<TokenType>(rule["type"].get<int>());
                    int precedence = rule["precedence"];
                    
                    config.addTokenRule(TokenRule(name, pattern, type, precedence));
                }
            }
        }
        
        return config;
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing language configuration: ") + e.what());
    }
}

// Helper function to sanitize strings for JSON
std::string sanitizeForJson(const std::string& input, bool isRegex = false) {
    std::string output;
    output.reserve(input.size());
    
    for (unsigned char c : input) {
        if (c < 128) { // ASCII range
            // Special handling for backslashes in regex patterns
            if (isRegex && c == '\\') {
                output += "\\\\"; // Double the backslashes for regex patterns
            } else {
                output += c;
            }
        } else {
            // Replace non-ASCII characters with their hex representation
            output += "\\x" + std::to_string(static_cast<int>(c));
        }
    }
    
    return output;
}

// Helper function to unescape regex patterns
std::string unescapeRegexPattern(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size() && input[i + 1] == '\\') {
            // Convert \\ back to \ for regex patterns
            output += '\\';
            i++; // Skip the next backslash
        } else {
            output += input[i];
        }
    }
    
    return output;
}

std::string ConfigLoader::languageToJsonString(const LanguageConfig& config) {
    json j;
    
    // Basic properties
    j["name"] = config.getName();
    j["version"] = config.getVersion();
    
    // Keywords
    j["keywords"] = json::array();
    for (const auto& keyword : config.keywordSets.keywords) {
        j["keywords"].push_back(sanitizeForJson(keyword));
    }
    
    // Types
    j["types"] = json::array();
    for (const auto& type : config.keywordSets.types) {
        j["types"].push_back(sanitizeForJson(type));
    }
    
    // Character sets
    j["characterSets"] = {
        {"identifierStart", sanitizeForJson(config.characterSets.identifierStart)},
        {"identifierContinue", sanitizeForJson(config.characterSets.identifierContinue)},
        {"operators", sanitizeForJson(config.characterSets.operators)},
        {"delimiters", sanitizeForJson(config.characterSets.delimiters)},
        {"whitespace", sanitizeForJson(config.characterSets.whitespace)}
    };
    
    // Comment configuration
    j["commentConfig"] = json::object();
    
    // Single line comment starts
    j["commentConfig"]["singleLineCommentStarts"] = json::array();
    for (const auto& start : config.commentConfig.singleLineCommentStarts) {
        j["commentConfig"]["singleLineCommentStarts"].push_back(sanitizeForJson(start));
    }
    
    // Multi-line comment delimiters
    j["commentConfig"]["multiLineCommentDelimiters"] = json::array();
    for (const auto& delim : config.commentConfig.multiLineCommentDelimiters) {
        j["commentConfig"]["multiLineCommentDelimiters"].push_back({
            {"start", sanitizeForJson(delim.first)},
            {"end", sanitizeForJson(delim.second)}
        });
    }
    
    // String configuration
    j["stringConfig"] = json::object();
    
    // String delimiters
    j["stringConfig"]["stringDelimiters"] = json::array();
    for (const auto& delim : config.stringConfig.stringDelimiters) {
        j["stringConfig"]["stringDelimiters"].push_back({
            {"start", sanitizeForJson(delim.first)},
            {"end", sanitizeForJson(delim.second)}
        });
    }
    
    // Character delimiters
    j["stringConfig"]["charDelimiters"] = json::array();
    for (const auto& delim : config.stringConfig.charDelimiters) {
        j["stringConfig"]["charDelimiters"].push_back({
            {"start", sanitizeForJson(delim.first)},
            {"end", sanitizeForJson(delim.second)}
        });
    }
    
    // Escape character and raw string prefix
    j["stringConfig"]["escapeChar"] = std::string(1, config.stringConfig.escapeChar);
    j["stringConfig"]["rawStringPrefix"] = config.stringConfig.rawStringPrefix;
    
    // Number configuration
    j["numberConfig"] = {
        {"decimalIntPattern", sanitizeForJson(config.numberConfig.decimalIntPattern)},
        {"floatingPointPattern", sanitizeForJson(config.numberConfig.floatingPointPattern)},
        {"hexPattern", sanitizeForJson(config.numberConfig.hexPattern)},
        {"octalPattern", sanitizeForJson(config.numberConfig.octalPattern)},
        {"binaryPattern", sanitizeForJson(config.numberConfig.binaryPattern)},
        {"scientificPattern", sanitizeForJson(config.numberConfig.scientificPattern)}
    };
    
    // Token rules
    j["tokenRules"] = json::array();
    for (const auto& rule : config.getTokenRules()) {
        j["tokenRules"].push_back({
            {"name", sanitizeForJson(rule.name)},
            {"pattern", sanitizeForJson(rule.getPatternString(), true)},
            {"type", static_cast<int>(rule.type)},
            {"precedence", rule.precedence}
        });
    }
    
    // Return the formatted JSON with indentation
    return j.dump(2);
}

// The following methods are no longer needed with the JSON library approach
void ConfigLoader::parseKeywordSets(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parseCharacterSets(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parseCommentConfig(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parseStringConfig(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parseNumberConfig(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parsePreprocessorConfig(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
}

void ConfigLoader::parseTokenRules(LanguageConfig& config, const std::string& jsonString) {
    // This method is no longer used with nlohmann/json
    (void)config;
    (void)jsonString;
} 