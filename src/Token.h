#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    INTEGER,
    FLOAT,
    IDENTIFIER,
    KEYWORD,
    OPERATOR,
    DELIMITER,
    STRING_LITERAL,
    EOF_TOKEN,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    Token(TokenType type, const std::string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
    
    std::string toString() const;
};

#endif // TOKEN_H 