#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <memory>
#include <fstream>
#include <stack>
#include <functional>
#include "Token.h"
#include "LanguageConfig.h"
#include "SymbolTable.h"

// Forward declaration
class TokenStream;

// Lexer class with advanced features
class Lexer {
private:
    std::string source;
    std::string filename;
    size_t position;
    size_t line;
    size_t column;
    char currentChar;
    
    // Configuration
    LanguageConfig config;
    
    // Symbol table for tracking symbols
    std::shared_ptr<SymbolTable> symbolTable;
    
    // Preprocessor handling
    bool processPreprocessorDirectives;
    std::stack<bool> conditionalCompilationStack;
    std::unordered_map<std::string, std::string> macros;
    std::vector<std::string> includePaths;
    
    // Error handling
    struct Error {
        std::string message;
        int line;
        int column;
        std::string filename;
        
        Error(const std::string& message, int line, int column, const std::string& filename)
            : message(message), line(line), column(column), filename(filename) {}
    };
    std::vector<Error> errors;
    
    // State tracking
    enum class LexerState {
        NORMAL,
        IN_COMMENT,
        IN_STRING,
        IN_CHAR,
        IN_PREPROCESSOR
    };
    std::stack<LexerState> stateStack;
    
    // Comment handling
    std::string currentCommentStart;
    std::string currentCommentEnd;
    std::string currentComment;
    bool isDocComment;
    
    // String handling
    std::string currentStringStart;
    std::string currentStringEnd;
    std::string currentString;
    bool isRawString;
    bool hasEscapeSequences;
    
    // Helper methods
    void advance();
    void skipWhitespace();
    char peek(int offset = 1) const;
    std::string peekString(int length) const;
    
    // Token processing methods
    Token processIdentifier();
    Token processNumber();
    Token processStringLiteral();
    Token processCharLiteral();
    Token processOperator();
    Token processComment();
    Token processPreprocessor();
    
    // Advanced token recognition
    Token recognizeTokenFromRules();
    bool isStartOfComment(std::string& startDelimiter, std::string& endDelimiter, bool& isDoc);
    bool isStartOfString(std::string& startDelimiter, std::string& endDelimiter);
    
    // Preprocessor methods
    Token handlePreprocessorDirective();
    void handleInclude(const std::string& directive);
    void handleDefine(const std::string& directive);
    void handleUndefine(const std::string& directive);
    bool evaluatePreprocessorCondition(const std::string& condition);
    
    // Error reporting
    void reportError(const std::string& message);
    
public:
    // Constructor with configuration
    Lexer(const std::string& source, const std::string& filename = "");
    Lexer(const std::string& source, const LanguageConfig& config, const std::string& filename = "");
    
    // Set configuration options
    void setLanguageConfig(const LanguageConfig& config);
    void setPreprocessorEnabled(bool enabled);
    void addIncludePath(const std::string& path);
    
    // Symbol table access
    void setSymbolTable(std::shared_ptr<SymbolTable> table);
    std::shared_ptr<SymbolTable> getSymbolTable() const;
    
    // Core lexing methods
    Token getNextToken();
    std::vector<Token> tokenize();
    TokenStream createTokenStream();
    
    // File handling
    static std::string readFile(const std::string& filename);
    static Lexer fromFile(const std::string& filename, const LanguageConfig& config = LanguageConfig());
    
    // Error access
    bool hasErrors() const;
    const std::vector<Error>& getErrors() const;
    
    // Diagnostic methods
    std::string getErrorReport() const;
    std::string getTokenSummary(const std::vector<Token>& tokens) const;
};

// Token stream class for iterating over tokens with lookahead
class TokenStream {
private:
    std::vector<Token> tokens;
    size_t position;
    
public:
    TokenStream(const std::vector<Token>& tokens);
    
    // Stream operations
    Token current() const;
    Token next();
    Token peek(int offset = 1) const;
    bool hasMore() const;
    size_t remaining() const;
    void reset();
    
    // Context information
    std::vector<Token> getContext(int before, int after) const;
    
    // Navigation
    void advance(int count = 1);
    void skipUntil(TokenType type);
    void skipUntil(const std::string& lexeme);
    
    // Search operations
    bool lookingAt(TokenType type) const;
    bool lookingAt(const std::string& lexeme) const;
    bool lookingAtSequence(const std::vector<TokenType>& types) const;
    bool lookingAtSequence(const std::vector<std::string>& lexemes) const;
};

#endif // LEXER_H