#include "ConfigLoader.h"
#include "FileUtil.h"
#include <iostream>
#include <regex>
#include "../include/json.hpp"

using json = nlohmann::json;

LanguageConfig ConfigLoader::loadLanguageFromFile(const std::string& filename) {
    return loadLanguageFromString(readFileContents(filename));
}

bool ConfigLoader::saveLanguageToFile(const LanguageConfig& config, const std::string& filename) {
    return writeFileContents(filename, languageToJsonString(config));
}

LanguageConfig ConfigLoader::loadLanguageFromString(const std::string& jsonString) {
    try {

        json j = json::parse(jsonString);

        std::string name = j.value("name", "Custom");
        std::string version = j.value("version", "1.0");

        LanguageConfig config(name, version);

        if (j.contains("keywords") && j["keywords"].is_array()) {
            for (const auto& keyword : j["keywords"]) {
                if (keyword.is_string()) {
                    config.addKeyword(keyword);
                }
            }
        }

        if (j.contains("types") && j["types"].is_array()) {
            for (const auto& type : j["types"]) {
                if (type.is_string()) {
                    config.addType(type);
                }
            }
        }

        if (j.contains("builtins") && j["builtins"].is_array()) {
            for (const auto& builtin : j["builtins"])
                if (builtin.is_string()) config.addBuiltin(builtin);
        }

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

        if (j.contains("commentConfig") && j["commentConfig"].is_object()) {
            const auto& commentCfg = j["commentConfig"];

            if (commentCfg.contains("singleLineCommentStarts") && commentCfg["singleLineCommentStarts"].is_array()) {
                for (const auto& start : commentCfg["singleLineCommentStarts"]) {
                    if (start.is_string()) {
                        config.commentConfig.singleLineCommentStarts.push_back(start);
                    }
                }
            }

            if (commentCfg.contains("multiLineCommentDelimiters") && commentCfg["multiLineCommentDelimiters"].is_array()) {
                for (const auto& delim : commentCfg["multiLineCommentDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.commentConfig.multiLineCommentDelimiters.push_back({start, end});
                    }
                }
            }

            if (commentCfg.contains("docCommentStarts") && commentCfg["docCommentStarts"].is_array()) {
                for (const auto& start : commentCfg["docCommentStarts"]) {
                    if (start.is_string()) {
                        config.commentConfig.docCommentStarts.push_back(start);
                    }
                }
            }

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

        if (j.contains("stringConfig") && j["stringConfig"].is_object()) {
            const auto& stringCfg = j["stringConfig"];

            if (stringCfg.contains("stringDelimiters") && stringCfg["stringDelimiters"].is_array()) {
                for (const auto& delim : stringCfg["stringDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.stringConfig.stringDelimiters.push_back({start, end});
                    }
                }
            }

            if (stringCfg.contains("charDelimiters") && stringCfg["charDelimiters"].is_array()) {
                for (const auto& delim : stringCfg["charDelimiters"]) {
                    if (delim.is_object() && delim.contains("start") && delim.contains("end")) {
                        std::string start = delim["start"];
                        std::string end = delim["end"];
                        config.stringConfig.charDelimiters.push_back({start, end});
                    }
                }
            }

            if (stringCfg.contains("escapeChar") && stringCfg["escapeChar"].is_string()) {
                std::string escapeStr = stringCfg["escapeChar"];
                if (!escapeStr.empty()) {
                    config.stringConfig.escapeChar = escapeStr[0];
                }
            }

            if (stringCfg.contains("rawStringPrefix") && stringCfg["rawStringPrefix"].is_string()) {
                config.stringConfig.rawStringPrefix = stringCfg["rawStringPrefix"];
            }
            config.stringConfig.cppRawStrings = stringCfg.value("cppRawStrings", false);
            if (stringCfg.contains("formattedStringPrefixes") && stringCfg["formattedStringPrefixes"].is_array())
                for (const auto& prefix : stringCfg["formattedStringPrefixes"])
                    if (prefix.is_string() && !prefix.get<std::string>().empty())
                        config.stringConfig.formattedStringPrefixes.push_back(prefix);
        }

        if (j.contains("preprocessorConfig") && j["preprocessorConfig"].is_object())
            config.preprocessorConfig.directivePrefix = j["preprocessorConfig"].value("directiveStart", "");

        if (j.contains("additionalOperators") && j["additionalOperators"].is_array())
            for (const auto& op : j["additionalOperators"])
                if (op.is_string() && !op.get<std::string>().empty()) config.additionalOperators.push_back(op);

        if (j.contains("tokenRules") && j["tokenRules"].is_array()) {
            for (const auto& rule : j["tokenRules"]) {
                if (rule.is_object() &&
                    rule.contains("name") && rule.contains("pattern") &&
                    rule.contains("type") && rule.contains("precedence")) {

                    std::string name = rule["name"];
                    std::string pattern = rule["pattern"];

                    int typeValue = rule["type"].get<int>();
                    if (typeValue < 0 || typeValue > static_cast<int>(TokenType::COMMENT))
                        throw std::runtime_error("Invalid token rule type");
                    TokenType type = static_cast<TokenType>(typeValue);
                    int precedence = rule["precedence"];

                    config.addTokenRule(TokenRule(name, pattern, type, precedence));
                }
            }
        }

        for (const auto& start : config.commentConfig.singleLineCommentStarts)
            if (start.empty()) throw std::runtime_error("Empty comment delimiter");
        for (const auto& start : config.commentConfig.docCommentStarts)
            if (start.empty()) throw std::runtime_error("Empty comment delimiter");
        auto validateDelimiters = [](const auto& delimiters) {
            for (const auto& delimiter : delimiters)
                if (delimiter.first.empty() || delimiter.second.empty())
                    throw std::runtime_error("Empty delimiter");
        };
        validateDelimiters(config.commentConfig.multiLineCommentDelimiters);
        validateDelimiters(config.commentConfig.docCommentDelimiters);
        validateDelimiters(config.stringConfig.stringDelimiters);
        validateDelimiters(config.stringConfig.charDelimiters);
        return config;
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing language configuration: ") + e.what());
    }
}

std::string ConfigLoader::languageToJsonString(const LanguageConfig& config) {
    json j;

    j["name"] = config.getName();
    j["version"] = config.getVersion();

    j["keywords"] = json::array();
    for (const auto& keyword : config.keywordSets.keywords) {
        j["keywords"].push_back(keyword);
    }

    j["types"] = json::array();
    for (const auto& type : config.keywordSets.types) {
        j["types"].push_back(type);
    }

    j["builtins"] = json::array();
    for (const auto& builtin : config.keywordSets.builtins) j["builtins"].push_back(builtin);

    j["characterSets"] = {
        {"identifierStart", config.characterSets.identifierStart},
        {"identifierContinue", config.characterSets.identifierContinue},
        {"operators", config.characterSets.operators},
        {"delimiters", config.characterSets.delimiters},
        {"whitespace", config.characterSets.whitespace}
    };

    j["commentConfig"] = json::object();

    j["commentConfig"]["singleLineCommentStarts"] = json::array();
    for (const auto& start : config.commentConfig.singleLineCommentStarts) {
        j["commentConfig"]["singleLineCommentStarts"].push_back(start);
    }

    j["commentConfig"]["multiLineCommentDelimiters"] = json::array();
    for (const auto& delim : config.commentConfig.multiLineCommentDelimiters) {
        j["commentConfig"]["multiLineCommentDelimiters"].push_back({
            {"start", delim.first},
            {"end", delim.second}
        });
    }

    j["commentConfig"]["docCommentStarts"] = config.commentConfig.docCommentStarts;
    j["commentConfig"]["docCommentDelimiters"] = json::array();
    for (const auto& delim : config.commentConfig.docCommentDelimiters)
        j["commentConfig"]["docCommentDelimiters"].push_back({{"start", delim.first}, {"end", delim.second}});

    j["stringConfig"] = json::object();

    j["stringConfig"]["stringDelimiters"] = json::array();
    for (const auto& delim : config.stringConfig.stringDelimiters) {
        j["stringConfig"]["stringDelimiters"].push_back({
            {"start", delim.first},
            {"end", delim.second}
        });
    }

    j["stringConfig"]["charDelimiters"] = json::array();
    for (const auto& delim : config.stringConfig.charDelimiters) {
        j["stringConfig"]["charDelimiters"].push_back({
            {"start", delim.first},
            {"end", delim.second}
        });
    }

    j["stringConfig"]["escapeChar"] = std::string(1, config.stringConfig.escapeChar);
    j["stringConfig"]["rawStringPrefix"] = config.stringConfig.rawStringPrefix;
    j["stringConfig"]["formattedStringPrefixes"] = config.stringConfig.formattedStringPrefixes;
    j["stringConfig"]["cppRawStrings"] = config.stringConfig.cppRawStrings;
    j["preprocessorConfig"]["directiveStart"] = config.preprocessorConfig.directivePrefix;
    j["additionalOperators"] = config.additionalOperators;

    j["tokenRules"] = json::array();
    for (const auto& rule : config.getTokenRules()) {
        j["tokenRules"].push_back({
            {"name", rule.name},
            {"pattern", rule.getPatternString()},
            {"type", static_cast<int>(rule.type)},
            {"precedence", rule.precedence}
        });
    }

    return j.dump(2);
}
