#include "Token.h"

std::string Token::toString() const {
    std::string typeStr;
    
    switch (type) {
        case TokenType::INTEGER: typeStr = "INTEGER"; break;
        case TokenType::FLOAT: typeStr = "FLOAT"; break;
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::KEYWORD: typeStr = "KEYWORD"; break;
        case TokenType::OPERATOR: typeStr = "OPERATOR"; break;
        case TokenType::DELIMITER: typeStr = "DELIMITER"; break;
        case TokenType::STRING_LITERAL: typeStr = "STRING_LITERAL"; break;
        case TokenType::EOF_TOKEN: typeStr = "EOF"; break;
        case TokenType::UNKNOWN: typeStr = "UNKNOWN"; break;
    }
    
    return typeStr + ": '" + lexeme + "' at line " + std::to_string(line) + 
           ", column " + std::to_string(column);
} 