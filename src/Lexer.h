#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "Token.h"

class Lexer {
private:
    std::string source;
    size_t position;
    size_t line;
    size_t column;
    char currentChar;
    
    std::unordered_map<std::string, TokenType> keywords;
    
    void advance();
    void skipWhitespace();
    char peek() const;
    
    Token processIdentifier();
    Token processNumber();
    Token processString();
    Token processOperator();
    
public:
    Lexer(const std::string& source);
    Token getNextToken();
    std::vector<Token> tokenize();
};

#endif // LEXER_H 