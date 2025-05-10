#ifndef LANGUAGE_CONFIG_H
#define LANGUAGE_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "Token.h"

// A rule for matching tokens with regex
struct TokenRule {
    std::string name;
    std::regex pattern;
    std::string patternString; // Store the original pattern string
    TokenType type;
    int precedence;
    
    TokenRule(const std::string& name, const std::string& pattern, TokenType type, int precedence = 0)
        : name(name), pattern(pattern), patternString(pattern), type(type), precedence(precedence) {}
        
    // Get the original pattern string
    const std::string& getPatternString() const { return patternString; }
};

// Container for a language's keyword sets
struct KeywordSets {
    std::unordered_set<std::string> keywords;
    std::unordered_set<std::string> types;
    std::unordered_set<std::string> directives;
    std::unordered_set<std::string> operators;
    std::unordered_set<std::string> builtins;
};

// Character sets for tokenization
struct CharacterSets {
    std::string identifierStart;
    std::string identifierContinue;
    std::string operators;
    std::string delimiters;
    std::string whitespace;
    
    // Helper method to check if a character belongs to a set
    bool isInSet(char c, const std::string& set) const {
        return set.find(c) != std::string::npos;
    }
};

// Comment configuration
struct CommentConfig {
    // Single line comments
    std::vector<std::string> singleLineCommentStarts;
    
    // Multi-line comments
    std::vector<std::pair<std::string, std::string>> multiLineCommentDelimiters;
    
    // Documentation comments
    std::vector<std::string> docCommentStarts;
    std::vector<std::pair<std::string, std::string>> docCommentDelimiters;
};

// String literal configuration
struct StringConfig {
    // Delimiters for string literals
    std::vector<std::pair<std::string, std::string>> stringDelimiters;
    
    // Character literal delimiters
    std::vector<std::pair<std::string, std::string>> charDelimiters;
    
    // Escape character
    char escapeChar;
    
    // Raw string prefix
    std::string rawStringPrefix;
};

// Number literal configuration
struct NumberConfig {
    std::string decimalIntPattern;
    std::string floatingPointPattern;
    std::string hexPattern;
    std::string octalPattern;
    std::string binaryPattern;
    std::string scientificPattern;
};

// Preprocessor configuration
struct PreprocessorConfig {
    std::string directivePrefix;
    std::unordered_map<std::string, PreprocessorAttribute::Type> directiveTypes;
};

// Main language configuration class
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
    NumberConfig numberConfig;
    PreprocessorConfig preprocessorConfig;
    
    // Constructors
    LanguageConfig() = default;
    LanguageConfig(const std::string& name, const std::string& version);
    
    // Configuration methods
    void addTokenRule(const TokenRule& rule);
    void addKeyword(const std::string& keyword);
    void addType(const std::string& type);
    void addOperator(const std::string& op);
    void addBuiltin(const std::string& builtin);
    
    // Access methods
    const std::string& getName() const { return name; }
    const std::string& getVersion() const { return version; }
    const std::vector<TokenRule>& getTokenRules() const { return tokenRules; }
    
    // Factory methods for predefined languages
    static LanguageConfig createCConfig();
    static LanguageConfig createCppConfig();
    static LanguageConfig createJavaConfig();
    static LanguageConfig createPythonConfig();
    static LanguageConfig createJavaScriptConfig();
};

#endif // LANGUAGE_CONFIG_H 