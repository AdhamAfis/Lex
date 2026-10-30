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

class TokenStream;

class Lexer {
private:
    std::string source;
    std::string filename;
    size_t position;
    size_t line;
    size_t column;
    char currentChar;

    LanguageConfig config;

    std::shared_ptr<SymbolTable> symbolTable;

    bool processPreprocessorDirectives;

    struct Error {
        std::string message;
        int line;
        int column;
        std::string filename;

        Error(const std::string& message, int line, int column, const std::string& filename)
            : message(message), line(line), column(column), filename(filename) {}
    };
    std::vector<Error> errors;

    enum class LexerState {
        NORMAL,
        IN_COMMENT,
        IN_STRING,
        IN_CHAR,
        IN_PREPROCESSOR
    };
    std::stack<LexerState> stateStack;

    std::string currentCommentStart;
    std::string currentCommentEnd;
    std::string currentComment;
    bool isDocComment;
    SourceLocation tokenStart;

    std::string currentStringStart;
    std::string currentStringEnd;
    std::string currentString;
    bool isRawString;
    bool hasEscapeSequences;

    void advance();
    void skipWhitespace();
    char peek(int offset = 1) const;
    std::string peekString(int length) const;

    Token processIdentifier();
    Token classifyIdentifier(const std::string& lexeme, int startLine, int startColumn);
    Token processNumber();
    Token processStringLiteral();
    Token processCharLiteral();
    Token processOperator();
    Token processComment();
    Token processPreprocessor();

    Token recognizeTokenFromRules();
    bool isStartOfComment(std::string& startDelimiter, std::string& endDelimiter, bool& isDoc);
    bool isStartOfString(std::string& startDelimiter, std::string& endDelimiter);

    void reportError(const std::string& message);

public:

    Lexer(const std::string& source, const std::string& filename = "");
    Lexer(const std::string& source, const LanguageConfig& config, const std::string& filename = "");

    void setLanguageConfig(const LanguageConfig& config);
    void setPreprocessorEnabled(bool enabled);

    void setSymbolTable(std::shared_ptr<SymbolTable> table);
    std::shared_ptr<SymbolTable> getSymbolTable() const;

    Token getNextToken();
    std::vector<Token> tokenize();
    TokenStream createTokenStream();

    static std::string readFile(const std::string& filename);
    static Lexer fromFile(const std::string& filename, const LanguageConfig& config = LanguageConfig());

    bool hasErrors() const;
    const std::vector<Error>& getErrors() const;

    std::string getErrorReport() const;
    std::string getTokenSummary(const std::vector<Token>& tokens) const;
};

class TokenStream {
private:
    std::vector<Token> tokens;
    size_t position;

public:
    TokenStream(const std::vector<Token>& tokens);

    Token current() const;
    Token next();
    Token peek(int offset = 1) const;
    bool hasMore() const;
    size_t remaining() const;
    void reset();

    std::vector<Token> getContext(int before, int after) const;

    void advance(int count = 1);
    void skipUntil(TokenType type);
    void skipUntil(const std::string& lexeme);

    bool lookingAt(TokenType type) const;
    bool lookingAt(const std::string& lexeme) const;
    bool lookingAtSequence(const std::vector<TokenType>& types) const;
    bool lookingAtSequence(const std::vector<std::string>& lexemes) const;
};

#endif
