#ifndef LANGUAGE_CONFIG_H
#define LANGUAGE_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "Token.h"

struct TokenRule {
    std::string name;
    std::regex pattern;
    std::string patternString;
    TokenType type;
    int precedence;

    TokenRule(const std::string& name, const std::string& pattern, TokenType type, int precedence = 0)
        : name(name), pattern(pattern), patternString(pattern), type(type), precedence(precedence) {}

    const std::string& getPatternString() const { return patternString; }
};

struct KeywordSets {
    std::unordered_set<std::string> keywords;
    std::unordered_set<std::string> types;
    std::unordered_set<std::string> directives;
    std::unordered_set<std::string> operators;
    std::unordered_set<std::string> builtins;
};

struct CharacterSets {
    std::string identifierStart;
    std::string identifierContinue;
    std::string operators;
    std::string delimiters;
    std::string whitespace;

    bool isInSet(char c, const std::string& set) const {
        return set.find(c) != std::string::npos;
    }
};

struct CommentConfig {

    std::vector<std::string> singleLineCommentStarts;

    std::vector<std::pair<std::string, std::string>> multiLineCommentDelimiters;

    std::vector<std::string> docCommentStarts;
    std::vector<std::pair<std::string, std::string>> docCommentDelimiters;
};

struct StringConfig {

    std::vector<std::pair<std::string, std::string>> stringDelimiters;

    std::vector<std::pair<std::string, std::string>> charDelimiters;

    char escapeChar = '\\';

    std::string rawStringPrefix;
    std::vector<std::string> formattedStringPrefixes;
    bool cppRawStrings = false;
};

struct PreprocessorConfig {
    std::string directivePrefix;
    std::unordered_map<std::string, PreprocessorAttribute::Type> directiveTypes;
};

class LanguageConfig {
private:
    std::string name;
    std::string version;
    std::vector<TokenRule> tokenRules;

public:
    KeywordSets keywordSets;
    CharacterSets characterSets;
    CommentConfig commentConfig;
    StringConfig stringConfig;
    PreprocessorConfig preprocessorConfig;
    std::vector<std::string> additionalOperators;

    LanguageConfig() = default;
    LanguageConfig(const std::string& name, const std::string& version);

    void addTokenRule(const TokenRule& rule);
    void addKeyword(const std::string& keyword);
    void addType(const std::string& type);
    void addOperator(const std::string& op);
    void addBuiltin(const std::string& builtin);

    const std::string& getName() const { return name; }
    const std::string& getVersion() const { return version; }
    const std::vector<TokenRule>& getTokenRules() const { return tokenRules; }

    static LanguageConfig createCConfig();
    static LanguageConfig createCppConfig();
    static LanguageConfig createJavaConfig();
    static LanguageConfig createPythonConfig();
    static LanguageConfig createJavaScriptConfig();
};

#endif
