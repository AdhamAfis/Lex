#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), position(0), line(1), column(1) {
    // Initialize keywords
    keywords["int"] = TokenType::KEYWORD;
    keywords["float"] = TokenType::KEYWORD;
    keywords["if"] = TokenType::KEYWORD;
    keywords["else"] = TokenType::KEYWORD;
    keywords["while"] = TokenType::KEYWORD;
    keywords["for"] = TokenType::KEYWORD;
    keywords["return"] = TokenType::KEYWORD;
    
    currentChar = position < source.length() ? source[position] : '\0';
}

void Lexer::advance() {
    if (currentChar == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    
    position++;
    currentChar = position < source.length() ? source[position] : '\0';
}

void Lexer::skipWhitespace() {
    while (currentChar != '\0' && std::isspace(currentChar)) {
        advance();
    }
}

char Lexer::peek() const {
    if (position + 1 >= source.length()) {
        return '\0';
    }
    return source[position + 1];
}

Token Lexer::processIdentifier() {
    std::string lexeme;
    size_t startColumn = column;
    
    while (currentChar != '\0' && (std::isalnum(currentChar) || currentChar == '_')) {
        lexeme += currentChar;
        advance();
    }
    
    // Check if it's a keyword
    if (keywords.find(lexeme) != keywords.end()) {
        return Token(TokenType::KEYWORD, lexeme, line, startColumn);
    }
    
    return Token(TokenType::IDENTIFIER, lexeme, line, startColumn);
}

Token Lexer::processNumber() {
    std::string lexeme;
    size_t startColumn = column;
    bool isFloat = false;
    
    while (currentChar != '\0' && (std::isdigit(currentChar) || currentChar == '.')) {
        if (currentChar == '.') {
            if (isFloat) {
                break; // Second decimal point, stop processing
            }
            isFloat = true;
        }
        
        lexeme += currentChar;
        advance();
    }
    
    return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, lexeme, line, startColumn);
}

Token Lexer::processString() {
    std::string lexeme;
    size_t startColumn = column;
    
    // Skip the opening quote
    advance();
    
    while (currentChar != '\0' && currentChar != '"') {
        lexeme += currentChar;
        advance();
    }
    
    // Skip the closing quote
    if (currentChar == '"') {
        advance();
    }
    
    return Token(TokenType::STRING_LITERAL, lexeme, line, startColumn);
}

Token Lexer::processOperator() {
    std::string lexeme;
    size_t startColumn = column;
    
    // Single-character operators
    lexeme += currentChar;
    
    // Check for multi-character operators
    if ((currentChar == '=' && peek() == '=') ||
        (currentChar == '!' && peek() == '=') ||
        (currentChar == '<' && peek() == '=') ||
        (currentChar == '>' && peek() == '=') ||
        (currentChar == '&' && peek() == '&') ||
        (currentChar == '|' && peek() == '|')) {
        advance();
        lexeme += currentChar;
    }
    
    advance();
    return Token(TokenType::OPERATOR, lexeme, line, startColumn);
}

Token Lexer::getNextToken() {
    skipWhitespace();
    
    if (currentChar == '\0') {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    
    // Identify token type
    if (std::isalpha(currentChar) || currentChar == '_') {
        return processIdentifier();
    }
    
    if (std::isdigit(currentChar)) {
        return processNumber();
    }
    
    if (currentChar == '"') {
        return processString();
    }
    
    // Check for operators
    if (currentChar == '+' || currentChar == '-' || currentChar == '*' || 
        currentChar == '/' || currentChar == '%' || currentChar == '=' || 
        currentChar == '!' || currentChar == '<' || currentChar == '>' || 
        currentChar == '&' || currentChar == '|') {
        return processOperator();
    }
    
    // Delimiters
    if (currentChar == '(' || currentChar == ')' || currentChar == '{' || 
        currentChar == '}' || currentChar == '[' || currentChar == ']' || 
        currentChar == ';' || currentChar == ',' || currentChar == '.') {
        Token token(TokenType::DELIMITER, std::string(1, currentChar), line, column);
        advance();
        return token;
    }
    
    // Unknown character
    Token token(TokenType::UNKNOWN, std::string(1, currentChar), line, column);
    advance();
    return token;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token = getNextToken();
    
    while (token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(token);
        token = getNextToken();
    }
    
    tokens.push_back(token); // Add EOF token
    return tokens;
} 