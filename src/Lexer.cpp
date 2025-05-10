#include "Lexer.h"
#include <cctype>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

// TokenStream implementation
TokenStream::TokenStream(const std::vector<Token>& tokens) 
    : tokens(tokens), position(0) {}

Token TokenStream::current() const {
    if (position < tokens.size()) {
        return tokens[position];
    }
    // Return EOF token if at the end
    static Token eofToken(TokenType::EOF_TOKEN, "", 0, 0);
    return eofToken;
}

Token TokenStream::next() {
    Token token = current();
    advance();
    return token;
}

Token TokenStream::peek(int offset) const {
    size_t peekPos = position + offset;
    if (peekPos < tokens.size()) {
        return tokens[peekPos];
    }
    // Return EOF token if beyond the end
    static Token eofToken(TokenType::EOF_TOKEN, "", 0, 0);
    return eofToken;
}

bool TokenStream::hasMore() const {
    return position < tokens.size();
}

size_t TokenStream::remaining() const {
    if (position >= tokens.size()) {
        return 0;
    }
    return tokens.size() - position;
}

void TokenStream::reset() {
    position = 0;
}

std::vector<Token> TokenStream::getContext(int before, int after) const {
    std::vector<Token> context;
    
    // Calculate the start and end positions
    int startPos = std::max(0, static_cast<int>(position) - before);
    int endPos = std::min(tokens.size(), position + after + 1);
    
    // Extract the tokens
    for (int i = startPos; i < endPos; ++i) {
        context.push_back(tokens[i]);
    }
    
    return context;
}

void TokenStream::advance(int count) {
    position = std::min(tokens.size(), position + count);
}

void TokenStream::skipUntil(TokenType type) {
    while (hasMore() && current().type != type) {
        advance();
    }
}

void TokenStream::skipUntil(const std::string& lexeme) {
    while (hasMore() && current().lexeme != lexeme) {
        advance();
    }
}

bool TokenStream::lookingAt(TokenType type) const {
    return hasMore() && current().type == type;
}

bool TokenStream::lookingAt(const std::string& lexeme) const {
    return hasMore() && current().lexeme == lexeme;
}

bool TokenStream::lookingAtSequence(const std::vector<TokenType>& types) const {
    if (types.size() > remaining()) {
        return false;
    }
    
    for (size_t i = 0; i < types.size(); ++i) {
        if (peek(i).type != types[i]) {
            return false;
        }
    }
    
    return true;
}

bool TokenStream::lookingAtSequence(const std::vector<std::string>& lexemes) const {
    if (lexemes.size() > remaining()) {
        return false;
    }
    
    for (size_t i = 0; i < lexemes.size(); ++i) {
        if (peek(i).lexeme != lexemes[i]) {
            return false;
        }
    }
    
    return true;
}

// Lexer implementation
Lexer::Lexer(const std::string& source, const std::string& filename)
    : source(source), filename(filename), position(0), line(1), column(1),
      processPreprocessorDirectives(true), isDocComment(false), 
      isRawString(false), hasEscapeSequences(false) {
    
    // Initialize with default C++ config
    config = LanguageConfig::createCppConfig();
    
    // Initialize the symbol table
    symbolTable = std::make_shared<SymbolTable>();
    
    // Initialize state
    stateStack.push(LexerState::NORMAL);
    
    // Set the current character if source is not empty
    currentChar = !source.empty() ? source[0] : '\0';
}

Lexer::Lexer(const std::string& source, const LanguageConfig& config, const std::string& filename)
    : source(source), filename(filename), position(0), line(1), column(1), config(config),
      processPreprocessorDirectives(true), isDocComment(false), 
      isRawString(false), hasEscapeSequences(false) {
    
    // Initialize the symbol table
    symbolTable = std::make_shared<SymbolTable>();
    
    // Initialize state
    stateStack.push(LexerState::NORMAL);
    
    // Set the current character if source is not empty
    currentChar = !source.empty() ? source[0] : '\0';
}

void Lexer::setLanguageConfig(const LanguageConfig& newConfig) {
    config = newConfig;
}

void Lexer::setPreprocessorEnabled(bool enabled) {
    processPreprocessorDirectives = enabled;
}

void Lexer::addIncludePath(const std::string& path) {
    includePaths.push_back(path);
}

void Lexer::setSymbolTable(std::shared_ptr<SymbolTable> table) {
    symbolTable = table;
}

std::shared_ptr<SymbolTable> Lexer::getSymbolTable() const {
    return symbolTable;
}

// Helper method implementations
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

char Lexer::peek(int offset) const {
    size_t peekPos = position + offset;
    if (peekPos >= source.length()) {
        return '\0';
    }
    return source[peekPos];
}

std::string Lexer::peekString(int length) const {
    if (position + length > source.length()) {
        return source.substr(position);
    }
    return source.substr(position, length);
}

// Error reporting
void Lexer::reportError(const std::string& message) {
    errors.push_back(Error(message, line, column, filename));
}

bool Lexer::hasErrors() const {
    return !errors.empty();
}

const std::vector<Lexer::Error>& Lexer::getErrors() const {
    return errors;
}

std::string Lexer::getErrorReport() const {
    std::stringstream ss;
    
    if (errors.empty()) {
        ss << "No errors detected.\n";
        return ss.str();
    }
    
    ss << "Lexical errors (" << errors.size() << "):\n";
    
    for (const auto& error : errors) {
        ss << error.filename << ":" << error.line << ":" << error.column
           << ": error: " << error.message << "\n";
    }
    
    return ss.str();
}

// File handling
std::string Lexer::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Lexer Lexer::fromFile(const std::string& filename, const LanguageConfig& config) {
    std::string source = readFile(filename);
    return Lexer(source, config, filename);
}

// Token stream creation
TokenStream Lexer::createTokenStream() {
    return TokenStream(tokenize());
}

// Comment handling
bool Lexer::isStartOfComment(std::string& startDelimiter, std::string& endDelimiter, bool& isDoc) {
    for (const auto& delimiter : config.commentConfig.multiLineCommentDelimiters) {
        if (peekString(delimiter.first.length()) == delimiter.first) {
            startDelimiter = delimiter.first;
            endDelimiter = delimiter.second;
            isDoc = false;
            return true;
        }
    }
    
    for (const auto& start : config.commentConfig.singleLineCommentStarts) {
        if (peekString(start.length()) == start) {
            startDelimiter = start;
            endDelimiter = "\n";
            isDoc = false;
            return true;
        }
    }
    
    for (const auto& start : config.commentConfig.docCommentStarts) {
        if (peekString(start.length()) == start) {
            startDelimiter = start;
            endDelimiter = "\n";
            isDoc = true;
            return true;
        }
    }
    
    for (const auto& delimiter : config.commentConfig.docCommentDelimiters) {
        if (peekString(delimiter.first.length()) == delimiter.first) {
            startDelimiter = delimiter.first;
            endDelimiter = delimiter.second;
            isDoc = true;
            return true;
        }
    }
    
    return false;
}

// String handling
bool Lexer::isStartOfString(std::string& startDelimiter, std::string& endDelimiter) {
    // Check for raw string prefix
    if (!config.stringConfig.rawStringPrefix.empty()) {
        std::string potentialRawPrefix = peekString(config.stringConfig.rawStringPrefix.length());
        if (potentialRawPrefix == config.stringConfig.rawStringPrefix) {
            size_t offset = config.stringConfig.rawStringPrefix.length();
            
            // Look for any string delimiter after the raw prefix
            for (const auto& delimiter : config.stringConfig.stringDelimiters) {
                std::string potentialStart = peekString(offset + delimiter.first.length()).substr(offset);
                if (potentialStart == delimiter.first) {
                    startDelimiter = config.stringConfig.rawStringPrefix + delimiter.first;
                    endDelimiter = delimiter.second;
                    isRawString = true;
                    return true;
                }
            }
        }
    }
    
    // Check for regular string delimiters
    for (const auto& delimiter : config.stringConfig.stringDelimiters) {
        if (peekString(delimiter.first.length()) == delimiter.first) {
            startDelimiter = delimiter.first;
            endDelimiter = delimiter.second;
            isRawString = false;
            return true;
        }
    }
    
    // Check for character delimiters
    for (const auto& delimiter : config.stringConfig.charDelimiters) {
        if (peekString(delimiter.first.length()) == delimiter.first) {
            startDelimiter = delimiter.first;
            endDelimiter = delimiter.second;
            isRawString = false;
            return true;
        }
    }
    
    return false;
}

// Main token processing
Token Lexer::getNextToken() {
    if (stateStack.top() == LexerState::IN_COMMENT) {
        // Continue processing the current comment
        return processComment();
    }
    
    if (stateStack.top() == LexerState::IN_STRING) {
        // Continue processing the current string
        return processStringLiteral();
    }
    
    if (stateStack.top() == LexerState::IN_CHAR) {
        // Continue processing the current character literal
        return processCharLiteral();
    }
    
    skipWhitespace();
    
    if (currentChar == '\0') {
        return Token(TokenType::EOF_TOKEN, "", line, column, filename);
    }
    
    // Check for comments
    std::string commentStart, commentEnd;
    bool isDoc;
    if (isStartOfComment(commentStart, commentEnd, isDoc)) {
        this->currentCommentStart = commentStart;
        this->currentCommentEnd = commentEnd;
        this->isDocComment = isDoc;
        this->currentComment = "";
        
        // Skip the comment start delimiter
        for (size_t i = 0; i < commentStart.length(); ++i) {
            advance();
        }
        
        stateStack.push(LexerState::IN_COMMENT);
        return processComment();
    }
    
    // Check for strings
    std::string stringStart, stringEnd;
    if (isStartOfString(stringStart, stringEnd)) {
        this->currentStringStart = stringStart;
        this->currentStringEnd = stringEnd;
        this->currentString = "";
        this->hasEscapeSequences = false;
        
        // Skip the string start delimiter
        for (size_t i = 0; i < stringStart.length(); ++i) {
            advance();
        }
        
        stateStack.push(LexerState::IN_STRING);
        return processStringLiteral();
    }
    
    // Check for preprocessor directives
    if (processPreprocessorDirectives && currentChar == '#' && column == 1) {
        advance(); // Skip the '#'
        stateStack.push(LexerState::IN_PREPROCESSOR);
        return processPreprocessor();
    }
    
    // Try to match a token using regex rules first
    Token regexToken = recognizeTokenFromRules();
    if (regexToken.type != TokenType::UNKNOWN) {
        return regexToken;
    }
    
    // Identify token type with traditional methods
    if (std::isalpha(currentChar) || currentChar == '_') {
        return processIdentifier();
    }
    
    if (std::isdigit(currentChar)) {
        return processNumber();
    }
    
    // Check for operators
    if (config.characterSets.isInSet(currentChar, config.characterSets.operators)) {
        return processOperator();
    }
    
    // Check for delimiters
    if (config.characterSets.isInSet(currentChar, config.characterSets.delimiters)) {
        std::string delim(1, currentChar);
        Token token(TokenType::DELIMITER, delim, line, column, filename);
        
        // Determine more specific delimiter type
        if (currentChar == '(' || currentChar == ')') {
            token.type = TokenType::PARENTHESIS;
        } else if (currentChar == '[' || currentChar == ']') {
            token.type = TokenType::BRACKET;
        } else if (currentChar == '{' || currentChar == '}') {
            token.type = TokenType::BRACE;
        } else if (currentChar == ';') {
            token.type = TokenType::SEMICOLON;
        } else if (currentChar == ',') {
            token.type = TokenType::COMMA;
        } else if (currentChar == '.') {
            token.type = TokenType::DOT;
        }
        
        advance();
        return token;
    }
    
    // Unknown character
    std::string unknown(1, currentChar);
    Token token(TokenType::UNKNOWN, unknown, line, column, filename);
    reportError("Unexpected character: '" + unknown + "'");
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

// Token processing methods
Token Lexer::processIdentifier() {
    std::string lexeme;
    int startLine = line;
    int startColumn = column;
    
    while (currentChar != '\0' && 
           (std::isalnum(currentChar) || currentChar == '_')) {
        lexeme += currentChar;
        advance();
    }
    
    // Check if it's a keyword
    if (config.keywordSets.keywords.count(lexeme) > 0) {
        return Token(TokenType::KEYWORD, lexeme, startLine, startColumn, filename);
    }
    
    // Check if it's a type
    if (config.keywordSets.types.count(lexeme) > 0) {
        // Still a keyword, but we could add a TYPE token in the future
        return Token(TokenType::KEYWORD, lexeme, startLine, startColumn, filename);
    }
    
    // Check if it's a built-in
    if (config.keywordSets.builtins.count(lexeme) > 0) {
        // Could add a BUILTIN token type in the future
        auto attribute = std::make_shared<IdentifierAttribute>(true, false, "built-in");
        Token token(TokenType::IDENTIFIER, lexeme, startLine, startColumn, filename);
        token.attribute = attribute;
        return token;
    }
    
    // Regular identifier
    // Add to symbol table
    if (symbolTable) {
        Symbol* symbol = symbolTable->currentScope->lookup(lexeme);
        
        if (!symbol) {
            // New identifier, create symbol
            symbol = symbolTable->addSymbol(lexeme, SymbolKind::UNKNOWN, "", startLine, startColumn);
        } else {
            // Mark as used
            symbol->setUsed(true);
        }
        
        auto attribute = std::make_shared<IdentifierAttribute>(
            symbol->getIsDefined(), false, symbol->getScope() ? symbol->getScope()->getName() : "");
        
        Token token(TokenType::IDENTIFIER, lexeme, startLine, startColumn, filename);
        token.attribute = attribute;
        return token;
    }
    
    return Token(TokenType::IDENTIFIER, lexeme, startLine, startColumn, filename);
}

Token Lexer::processNumber() {
    // Implementation of advanced number processing
    std::string lexeme;
    int startLine = line;
    int startColumn = column;
    bool isFloat = false;
    bool isHex = false;
    bool isOctal = false;
    bool isBinary = false;
    bool isScientific = false;
    
    // Check for hex, octal, or binary prefix
    if (currentChar == '0') {
        lexeme += currentChar;
        advance();
        
        if (currentChar == 'x' || currentChar == 'X') {
            // Hexadecimal
            isHex = true;
            lexeme += currentChar;
            advance();
            
            // Process hex digits
            while (std::isxdigit(currentChar)) {
                lexeme += currentChar;
                advance();
            }
            
            auto attribute = std::make_shared<NumberAttribute>(NumberAttribute::Base::HEX);
            Token token(TokenType::HEX, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        } 
        else if (currentChar == 'b' || currentChar == 'B') {
            // Binary
            isBinary = true;
            lexeme += currentChar;
            advance();
            
            // Process binary digits
            while (currentChar == '0' || currentChar == '1') {
                lexeme += currentChar;
                advance();
            }
            
            auto attribute = std::make_shared<NumberAttribute>(NumberAttribute::Base::BINARY);
            Token token(TokenType::BINARY, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
        else if (currentChar == 'o' || currentChar == 'O') {
            // New-style octal (C++14, Python)
            isOctal = true;
            lexeme += currentChar;
            advance();
            
            // Process octal digits
            while ('0' <= currentChar && currentChar <= '7') {
                lexeme += currentChar;
                advance();
            }
            
            auto attribute = std::make_shared<NumberAttribute>(NumberAttribute::Base::OCTAL);
            Token token(TokenType::OCTAL, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
        else if ('0' <= currentChar && currentChar <= '7') {
            // Old-style octal (C/C++)
            isOctal = true;
            
            // Process octal digits
            while ('0' <= currentChar && currentChar <= '7') {
                lexeme += currentChar;
                advance();
            }
            
            auto attribute = std::make_shared<NumberAttribute>(NumberAttribute::Base::OCTAL);
            Token token(TokenType::OCTAL, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
        
        // Just a zero
        if (currentChar != '.') {
            auto attribute = std::make_shared<NumberAttribute>();
            Token token(TokenType::INTEGER, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
    }
    
    // Regular decimal number
    while (std::isdigit(currentChar)) {
        lexeme += currentChar;
        advance();
    }
    
    // Check for decimal point
    if (currentChar == '.') {
        lexeme += currentChar;
        advance();
        isFloat = true;
        
        // Process fractional part
        while (std::isdigit(currentChar)) {
            lexeme += currentChar;
            advance();
        }
    }
    
    // Check for scientific notation
    if (currentChar == 'e' || currentChar == 'E') {
        lexeme += currentChar;
        advance();
        isScientific = true;
        
        // Optional sign
        if (currentChar == '+' || currentChar == '-') {
            lexeme += currentChar;
            advance();
        }
        
        // Exponent must have at least one digit
        if (!std::isdigit(currentChar)) {
            reportError("Invalid scientific notation: exponent has no digits");
            auto attribute = std::make_shared<NumberAttribute>(
                NumberAttribute::Base::DECIMAL, isFloat, isScientific);
            Token token(TokenType::ERROR, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
        
        // Process exponent
        while (std::isdigit(currentChar)) {
            lexeme += currentChar;
            advance();
        }
        
        auto attribute = std::make_shared<NumberAttribute>(
            NumberAttribute::Base::DECIMAL, isFloat, isScientific);
        Token token(TokenType::SCIENTIFIC, lexeme, startLine, startColumn, filename);
        token.attribute = attribute;
        return token;
    }
    
    // Check for type suffixes (f, l, etc.)
    if (std::isalpha(currentChar)) {
        lexeme += currentChar;
        advance();
    }
    
    // Create the token based on the number type
    auto attribute = std::make_shared<NumberAttribute>(
        NumberAttribute::Base::DECIMAL, isFloat, isScientific);
    
    Token token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, 
                lexeme, startLine, startColumn, filename);
    token.attribute = attribute;
    return token;
}

// Uses regex to match tokens based on rules
Token Lexer::recognizeTokenFromRules() {
    // Get the current position in the source
    std::string remainingSource = source.substr(position);
    
    // Try each rule in order of precedence
    for (const auto& rule : config.getTokenRules()) {
        std::smatch match;
        
        // Try to match at the current position
        if (std::regex_search(remainingSource, match, rule.pattern, 
                              std::regex_constants::match_continuous)) {
            // Get the matched text
            std::string lexeme = match.str();
            
            // Create the token
            Token token(rule.type, lexeme, line, column, filename);
            
            // Advance past the matched text
            for (size_t i = 0; i < lexeme.length(); ++i) {
                advance();
            }
            
            return token;
        }
    }
    
    // No match found
    return Token(TokenType::UNKNOWN, "", line, column, filename);
}

// Implementation of missing methods

Token Lexer::processComment() {
    int startLine = line;
    int startColumn = column;
    
    // Process multi-line comment
    if (currentCommentEnd != "\n") {
        // Look for the end delimiter
        while (currentChar != '\0') {
            // Check if we've reached the end delimiter
            if (peekString(currentCommentEnd.length()) == currentCommentEnd) {
                // Skip the end delimiter
                for (size_t i = 0; i < currentCommentEnd.length(); ++i) {
                    currentComment += currentChar;
                    advance();
                }
                
                // Create token and pop state
                Token token(TokenType::COMMENT, currentComment, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
            
            // Add current character to comment and advance
            currentComment += currentChar;
            advance();
            
            // Handle EOF in comment (syntax error)
            if (currentChar == '\0') {
                reportError("Unterminated comment");
                Token token(TokenType::ERROR, currentComment, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    } 
    // Process single-line comment
    else {
        // Read until end of line or EOF
        while (currentChar != '\0' && currentChar != '\n') {
            currentComment += currentChar;
            advance();
        }
        
        // Create token and pop state
        Token token(TokenType::COMMENT, currentComment, startLine, startColumn, filename);
        stateStack.pop();
        return token;
    }
    
    // This should never be reached
    return Token(TokenType::ERROR, "", line, column, filename);
}

Token Lexer::processStringLiteral() {
    int startLine = line;
    int startColumn = column;
    
    // Process string content
    while (currentChar != '\0') {
        // Check for end delimiter
        if (!isRawString && currentChar == '\\') {
            // Handle escape sequence
            currentString += currentChar;
            advance();
            
            if (currentChar != '\0') {
                currentString += currentChar;
                hasEscapeSequences = true;
                advance();
            }
        } 
        // Check for string end
        else if (peekString(currentStringEnd.length()) == currentStringEnd) {
            // Skip the end delimiter
            for (size_t i = 0; i < currentStringEnd.length(); ++i) {
                advance();
            }
            
            // Create token with appropriate type and pop state
            TokenType type = TokenType::STRING_LITERAL;
            
            Token token(type, currentString, startLine, startColumn, filename);
            
            // Add escape sequence information as attribute
            if (hasEscapeSequences) {
                auto attribute = std::make_shared<StringAttribute>(true);
                token.attribute = attribute;
            }
            
            stateStack.pop();
            return token;
        } 
        else {
            // Add current character to string and advance
            currentString += currentChar;
            advance();
            
            // Handle EOF in string (syntax error)
            if (currentChar == '\0') {
                reportError("Unterminated string literal");
                Token token(TokenType::ERROR, currentString, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    }
    
    // This should never be reached
    return Token(TokenType::ERROR, "", line, column, filename);
}

Token Lexer::processCharLiteral() {
    int startLine = line;
    int startColumn = column;
    std::string charValue;
    
    // Process character content
    while (currentChar != '\0') {
        // Handle escape sequence
        if (currentChar == '\\') {
            charValue += currentChar;
            advance();
            
            if (currentChar != '\0') {
                charValue += currentChar;
                hasEscapeSequences = true;
                advance();
            }
        }
        // Check for char end
        else if (peekString(currentStringEnd.length()) == currentStringEnd) {
            // Skip the end delimiter
            for (size_t i = 0; i < currentStringEnd.length(); ++i) {
                advance();
            }
            
            // Create token
            Token token(TokenType::CHAR_LITERAL, charValue, startLine, startColumn, filename);
            
            // Add escape sequence information as attribute
            if (hasEscapeSequences) {
                auto attribute = std::make_shared<StringAttribute>(true);
                token.attribute = attribute;
            }
            
            stateStack.pop();
            return token;
        }
        else {
            // Add current character and advance
            charValue += currentChar;
            advance();
            
            // Handle EOF in char literal (syntax error)
            if (currentChar == '\0') {
                reportError("Unterminated character literal");
                Token token(TokenType::ERROR, charValue, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    }
    
    // This should never be reached
    return Token(TokenType::ERROR, "", line, column, filename);
}

Token Lexer::processOperator() {
    std::string op;
    int startLine = line;
    int startColumn = column;
    
    // Try to match the longest possible operator
    while (currentChar != '\0' && 
           config.characterSets.isInSet(currentChar, config.characterSets.operators)) {
        op += currentChar;
        advance();
        
        // Check if adding the next character would still form a valid operator
        if (currentChar != '\0' && 
            config.characterSets.isInSet(currentChar, config.characterSets.operators)) {
            std::string potentialOp = op + currentChar;
            
            // Check if this is still a valid operator
            bool isValidOp = false;
            for (const auto& opPair : config.getTokenRules()) {
                if (opPair.type == TokenType::OPERATOR || 
                    opPair.type == TokenType::ARITHMETIC_OPERATOR ||
                    opPair.type == TokenType::LOGICAL_OPERATOR ||
                    opPair.type == TokenType::BITWISE_OPERATOR ||
                    opPair.type == TokenType::COMPARISON_OPERATOR ||
                    opPair.type == TokenType::ASSIGNMENT_OPERATOR) {
                    
                    // Try to match the pattern
                    if (std::regex_match(potentialOp, opPair.pattern)) {
                        isValidOp = true;
                        break;
                    }
                }
            }
            
            // If not a valid operator, stop here
            if (!isValidOp) {
                break;
            }
        }
    }
    
    // Determine the specific operator type
    TokenType opType = TokenType::OPERATOR;
    
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || 
        op == "++" || op == "--") {
        opType = TokenType::ARITHMETIC_OPERATOR;
    }
    else if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || 
             op == "%=" || op == "&=" || op == "|=" || op == "^=" || op == "<<=" || op == ">>=") {
        opType = TokenType::ASSIGNMENT_OPERATOR;
    }
    else if (op == "&" || op == "|" || op == "^" || op == "~" || op == "<<" || op == ">>") {
        opType = TokenType::BITWISE_OPERATOR;
    }
    else if (op == "&&" || op == "||" || op == "!" || op == "and" || op == "or" || op == "not") {
        opType = TokenType::LOGICAL_OPERATOR;
    }
    else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" ||
             op == "===" || op == "!==") {
        opType = TokenType::COMPARISON_OPERATOR;
    }
    
    return Token(opType, op, startLine, startColumn, filename);
}

Token Lexer::processPreprocessor() {
    std::string directive;
    int startLine = line;
    int startColumn = column;
    
    // Skip leading whitespace
    skipWhitespace();
    
    // Read the directive name
    while (currentChar != '\0' && std::isalpha(currentChar)) {
        directive += currentChar;
        advance();
    }
    
    // Skip whitespace after directive name
    skipWhitespace();
    
    // Read the rest of the line
    std::string args;
    while (currentChar != '\0' && currentChar != '\n') {
        args += currentChar;
        advance();
    }
    
    // Process the directive (simplified implementation)
    std::string fullDirective = directive + " " + args;
    
    // Create token and pop state
    Token token(TokenType::PREPROCESSOR, fullDirective, startLine, startColumn, filename);
    stateStack.pop();
    return token;
} 