#include "Lexer.h"
#include "FileUtil.h"
#include <cctype>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

TokenStream::TokenStream(const std::vector<Token>& tokens)
    : tokens(tokens), position(0) {}

Token TokenStream::current() const {
    if (position < tokens.size()) {
        return tokens[position];
    }
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

    int startPos = std::max(0, static_cast<int>(position) - before);
    int endPos = std::min(tokens.size(), position + after + 1);

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

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source(source), filename(filename), position(0), line(1), column(1),
      processPreprocessorDirectives(true), isDocComment(false),
      isRawString(false), hasEscapeSequences(false) {
    if (source.find('\0') != std::string::npos) throw std::invalid_argument("Input contains a null byte");

    config = LanguageConfig::createCppConfig();

    symbolTable = std::make_shared<SymbolTable>();

    stateStack.push(LexerState::NORMAL);

    currentChar = !source.empty() ? source[0] : '\0';
}

Lexer::Lexer(const std::string& source, const LanguageConfig& config, const std::string& filename)
    : source(source), filename(filename), position(0), line(1), column(1), config(config),
      processPreprocessorDirectives(true), isDocComment(false),
      isRawString(false), hasEscapeSequences(false) {
    if (source.find('\0') != std::string::npos) throw std::invalid_argument("Input contains a null byte");

    symbolTable = std::make_shared<SymbolTable>();

    stateStack.push(LexerState::NORMAL);

    currentChar = !source.empty() ? source[0] : '\0';
}

void Lexer::setLanguageConfig(const LanguageConfig& newConfig) {
    config = newConfig;
}

void Lexer::setPreprocessorEnabled(bool enabled) {
    processPreprocessorDirectives = enabled;
}

void Lexer::setSymbolTable(std::shared_ptr<SymbolTable> table) {
    symbolTable = table;
}

std::shared_ptr<SymbolTable> Lexer::getSymbolTable() const {
    return symbolTable;
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

std::string Lexer::readFile(const std::string& filename) {
    return readFileContents(filename);
}

Lexer Lexer::fromFile(const std::string& filename, const LanguageConfig& config) {
    std::string source = readFile(filename);
    return Lexer(source, config, filename);
}

TokenStream Lexer::createTokenStream() {
    return TokenStream(tokenize());
}

bool Lexer::isStartOfComment(std::string& startDelimiter, std::string& endDelimiter, bool& isDoc) {
    auto match = [&](const std::string& start, const std::string& end, bool doc) {
        if (start.size() > startDelimiter.size() && peekString(start.size()) == start) {
            startDelimiter = start;
            endDelimiter = end;
            isDoc = doc;
        }
    };
    for (const auto& delimiter : config.commentConfig.docCommentDelimiters) {
        if (delimiter.first == "/**" && peekString(4) == "/**/") continue;
        match(delimiter.first, delimiter.second, true);
    }
    for (const auto& start : config.commentConfig.docCommentStarts) {
        if (start == "/**" && peekString(4) == "/**/") continue;
        match(start, "\n", true);
    }
    for (const auto& delimiter : config.commentConfig.multiLineCommentDelimiters)
        match(delimiter.first, delimiter.second, false);
    for (const auto& start : config.commentConfig.singleLineCommentStarts)
        match(start, "\n", false);
    return !startDelimiter.empty();
}

bool Lexer::isStartOfString(std::string& startDelimiter, std::string& endDelimiter) {
    for (const auto& prefix : config.stringConfig.formattedStringPrefixes) {
        for (const auto& delimiter : config.stringConfig.stringDelimiters) {
            if (peekString(prefix.size() + delimiter.first.size()) == prefix + delimiter.first) {
                startDelimiter = prefix + delimiter.first;
                endDelimiter = delimiter.second;
                isRawString = prefix.find_first_of("rR") != std::string::npos;
                return true;
            }
        }
    }
    if (config.stringConfig.cppRawStrings && peekString(2) == "R\"") {
        size_t open = source.find('(', position + 2);
        if (open != std::string::npos && open - (position + 2) <= 16) {
            std::string delimiter = source.substr(position + 2, open - position - 2);
            if (delimiter.find_first_of(" ()\\\t\r\n\v\f") == std::string::npos) {
                startDelimiter = source.substr(position, open - position + 1);
                endDelimiter = ")" + delimiter + "\"";
                isRawString = true;
                return true;
            }
        }
    }
    if (!config.stringConfig.rawStringPrefix.empty()) {
        std::string potentialRawPrefix = peekString(config.stringConfig.rawStringPrefix.length());
        if (potentialRawPrefix == config.stringConfig.rawStringPrefix ||
            (config.stringConfig.rawStringPrefix == "r" && potentialRawPrefix == "R")) {
            size_t offset = config.stringConfig.rawStringPrefix.length();

            for (const auto& delimiter : config.stringConfig.stringDelimiters) {
                std::string potentialStart = peekString(offset + delimiter.first.length()).substr(offset);
                if (potentialStart == delimiter.first) {
                    startDelimiter = potentialRawPrefix + delimiter.first;
                    endDelimiter = delimiter.second;
                    isRawString = true;
                    return true;
                }
            }
        }
    }

    for (const auto& delimiter : config.stringConfig.stringDelimiters) {
        if (peekString(delimiter.first.length()) == delimiter.first) {
            startDelimiter = delimiter.first;
            endDelimiter = delimiter.second;
            isRawString = false;
            return true;
        }
    }

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

Token Lexer::getNextToken() {
    if (stateStack.top() == LexerState::IN_COMMENT) {

        return processComment();
    }

    if (stateStack.top() == LexerState::IN_STRING) {

        return processStringLiteral();
    }

    if (stateStack.top() == LexerState::IN_CHAR) {

        return processCharLiteral();
    }

    skipWhitespace();

    if (currentChar == '\0') {
        return Token(TokenType::EOF_TOKEN, "", line, column, filename);
    }

    std::string commentStart, commentEnd;
    bool isDoc;
    if (isStartOfComment(commentStart, commentEnd, isDoc)) {
        tokenStart = SourceLocation(line, column, filename);
        this->currentCommentStart = commentStart;
        this->currentCommentEnd = commentEnd;
        this->isDocComment = isDoc;
        this->currentComment = "";

        for (size_t i = 0; i < commentStart.length(); ++i) {
            advance();
        }

        stateStack.push(LexerState::IN_COMMENT);
        return processComment();
    }

    std::string stringStart, stringEnd;
    if (isStartOfString(stringStart, stringEnd)) {
        tokenStart = SourceLocation(line, column, filename);
        this->currentStringStart = stringStart;
        this->currentStringEnd = stringEnd;
        this->currentString = "";
        this->hasEscapeSequences = false;

        for (size_t i = 0; i < stringStart.length(); ++i) {
            advance();
        }

        bool isChar = std::any_of(config.stringConfig.charDelimiters.begin(), config.stringConfig.charDelimiters.end(),
            [&](const auto& delimiter) { return delimiter.first == stringStart; });
        stateStack.push(isChar ? LexerState::IN_CHAR : LexerState::IN_STRING);
        return isChar ? processCharLiteral() : processStringLiteral();
    }

    if (processPreprocessorDirectives && !config.preprocessorConfig.directivePrefix.empty() &&
        peekString(config.preprocessorConfig.directivePrefix.size()) == config.preprocessorConfig.directivePrefix) {
        size_t lineStart = source.rfind('\n', position);
        lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;
        if (source.substr(lineStart, position - lineStart).find_first_not_of(" \t\r\f\v") == std::string::npos) {
            tokenStart = SourceLocation(line, column, filename);
            for (size_t i = 0; i < config.preprocessorConfig.directivePrefix.size(); ++i) advance();
            stateStack.push(LexerState::IN_PREPROCESSOR);
            return processPreprocessor();
        }
    }

    Token regexToken = recognizeTokenFromRules();
    if (regexToken.type != TokenType::UNKNOWN) {
        return regexToken;
    }

    if (std::isalpha(currentChar) || currentChar == '_') {
        return processIdentifier();
    }

    if (std::isdigit(currentChar)) {
        return processNumber();
    }

    if (config.characterSets.isInSet(currentChar, config.characterSets.operators)) {
        return processOperator();
    }

    if (config.characterSets.isInSet(currentChar, config.characterSets.delimiters)) {
        std::string delim(1, currentChar);
        Token token(TokenType::DELIMITER, delim, line, column, filename);

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

    tokens.push_back(token);
    return tokens;
}

Token Lexer::processIdentifier() {
    std::string lexeme;
    int startLine = line;
    int startColumn = column;

    while (currentChar != '\0' &&
           (std::isalnum(currentChar) || currentChar == '_')) {
        lexeme += currentChar;
        advance();
    }
    return classifyIdentifier(lexeme, startLine, startColumn);
}

Token Lexer::classifyIdentifier(const std::string& lexeme, int startLine, int startColumn) {

    if (config.keywordSets.keywords.count(lexeme) > 0) {
        return Token(TokenType::KEYWORD, lexeme, startLine, startColumn, filename);
    }

    if (config.keywordSets.types.count(lexeme) > 0) {

        return Token(TokenType::KEYWORD, lexeme, startLine, startColumn, filename);
    }

    if (config.keywordSets.builtins.count(lexeme) > 0) {

        auto attribute = std::make_shared<IdentifierAttribute>(true, false, "built-in");
        Token token(TokenType::IDENTIFIER, lexeme, startLine, startColumn, filename);
        token.attribute = attribute;
        return token;
    }

    if (symbolTable) {
        Symbol* symbol = symbolTable->currentScope->lookup(lexeme);

        if (!symbol) {

            symbol = symbolTable->addSymbol(lexeme, SymbolKind::UNKNOWN, "", startLine, startColumn);
        } else {

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

    std::string lexeme;
    int startLine = line;
    int startColumn = column;
    bool isFloat = false;
    bool isHex = false;
    bool isOctal = false;
    bool isBinary = false;
    bool isScientific = false;

    if (currentChar == '0') {
        lexeme += currentChar;
        advance();

        if (currentChar == 'x' || currentChar == 'X') {

            isHex = true;
            lexeme += currentChar;
            advance();

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

            isBinary = true;
            lexeme += currentChar;
            advance();

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

            isOctal = true;
            lexeme += currentChar;
            advance();

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

            isOctal = true;

            while ('0' <= currentChar && currentChar <= '7') {
                lexeme += currentChar;
                advance();
            }

            auto attribute = std::make_shared<NumberAttribute>(NumberAttribute::Base::OCTAL);
            Token token(TokenType::OCTAL, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }

        if (currentChar != '.') {
            auto attribute = std::make_shared<NumberAttribute>();
            Token token(TokenType::INTEGER, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }
    }

    while (std::isdigit(currentChar)) {
        lexeme += currentChar;
        advance();
    }

    if (currentChar == '.') {
        lexeme += currentChar;
        advance();
        isFloat = true;

        while (std::isdigit(currentChar)) {
            lexeme += currentChar;
            advance();
        }
    }

    if (currentChar == 'e' || currentChar == 'E') {
        lexeme += currentChar;
        advance();
        isScientific = true;

        if (currentChar == '+' || currentChar == '-') {
            lexeme += currentChar;
            advance();
        }

        if (!std::isdigit(currentChar)) {
            reportError("Invalid scientific notation: exponent has no digits");
            auto attribute = std::make_shared<NumberAttribute>(
                NumberAttribute::Base::DECIMAL, isFloat, isScientific);
            Token token(TokenType::ERROR, lexeme, startLine, startColumn, filename);
            token.attribute = attribute;
            return token;
        }

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

    if (std::isalpha(currentChar)) {
        lexeme += currentChar;
        advance();
    }

    auto attribute = std::make_shared<NumberAttribute>(
        NumberAttribute::Base::DECIMAL, isFloat, isScientific);

    Token token(isFloat ? TokenType::FLOAT : TokenType::INTEGER,
                lexeme, startLine, startColumn, filename);
    token.attribute = attribute;
    return token;
}

Token Lexer::recognizeTokenFromRules() {
    for (const auto& rule : config.getTokenRules()) {
        std::match_results<std::string::const_iterator> match;

        if (std::regex_search(source.cbegin() + position, source.cend(), match, rule.pattern,
                              std::regex_constants::match_continuous)) {
            std::string lexeme = match.str();
            if (lexeme.empty()) continue;
            Token token(rule.type, lexeme, line, column, filename);
            if (rule.type == TokenType::INTEGER || rule.type == TokenType::FLOAT) {
                size_t end = position + lexeme.size();
                bool badPrefix = lexeme == "0" && end < source.size() &&
                    (source[end] == 'x' || source[end] == 'X') &&
                    (end + 1 == source.size() || !std::isxdigit(static_cast<unsigned char>(source[end + 1])));
                bool badExponent = end < source.size() && (source[end] == 'e' || source[end] == 'E');
                if (badExponent) {
                    size_t digit = end + 1;
                    if (digit < source.size() && (source[digit] == '+' || source[digit] == '-')) ++digit;
                    badExponent = digit == source.size() || !std::isdigit(static_cast<unsigned char>(source[digit]));
                }
                if (badPrefix || badExponent) {
                    reportError("Invalid numeric literal");
                    token.type = TokenType::ERROR;
                }
            }

            for (size_t i = 0; i < lexeme.length(); ++i) {
                advance();
            }
            if (rule.type == TokenType::IDENTIFIER)
                return classifyIdentifier(lexeme, token.location.line, token.location.column);

            return token;
        }
    }

    return Token(TokenType::UNKNOWN, "", line, column, filename);
}

Token Lexer::processComment() {
    int startLine = tokenStart.line;
    int startColumn = tokenStart.column;

    if (currentCommentEnd != "\n") {

        while (currentChar != '\0') {
            if (peekString(currentCommentEnd.length()) == currentCommentEnd) {
                for (size_t i = 0; i < currentCommentEnd.length(); ++i) {
                    currentComment += currentChar;
                    advance();
                }

                Token token(TokenType::COMMENT, currentComment, startLine, startColumn, filename);
                token.attribute = std::make_shared<CommentAttribute>(isDocComment ? CommentAttribute::Type::DOC_COMMENT : CommentAttribute::Type::MULTI_LINE);
                stateStack.pop();
                return token;
            }

            currentComment += currentChar;
            advance();

            if (currentChar == '\0') {
                reportError("Unterminated comment");
                Token token(TokenType::ERROR, currentComment, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    }
    else {
        while (currentChar != '\0' && currentChar != '\n') {
            currentComment += currentChar;
            advance();
        }

        Token token(TokenType::COMMENT, currentComment, startLine, startColumn, filename);
        token.attribute = std::make_shared<CommentAttribute>(isDocComment ? CommentAttribute::Type::DOC_COMMENT : CommentAttribute::Type::SINGLE_LINE);
        stateStack.pop();
        return token;
    }

    reportError("Unterminated comment");
    stateStack.pop();
    return Token(TokenType::ERROR, currentComment, startLine, startColumn, filename);
}

Token Lexer::processStringLiteral() {
    int startLine = tokenStart.line;
    int startColumn = tokenStart.column;

    while (currentChar != '\0') {
        if ((currentChar == '\n' || currentChar == '\r') &&
            !(isRawString && config.stringConfig.cppRawStrings) &&
            currentStringEnd != "'''" && currentStringEnd != "\"\"\"" && currentStringEnd != "`") {
            reportError("Unterminated string literal");
            stateStack.pop();
            return Token(TokenType::ERROR, currentString, startLine, startColumn, filename);
        }
        if ((!isRawString || config.stringConfig.rawStringPrefix == "r") && currentChar == config.stringConfig.escapeChar) {

            currentString += currentChar;
            advance();

            if (currentChar != '\0') {
                currentString += currentChar;
                hasEscapeSequences = true;
                advance();
            }
        }
        else if (peekString(currentStringEnd.length()) == currentStringEnd) {
            for (size_t i = 0; i < currentStringEnd.length(); ++i) {
                advance();
            }

            TokenType type = TokenType::STRING_LITERAL;

            Token token(type, currentString, startLine, startColumn, filename);

            if (hasEscapeSequences) {
                auto attribute = std::make_shared<StringAttribute>(isRawString, hasEscapeSequences);
                token.attribute = attribute;
            }

            stateStack.pop();
            return token;
        }
        else {
            currentString += currentChar;
            advance();

            if (currentChar == '\0') {
                reportError("Unterminated string literal");
                Token token(TokenType::ERROR, currentString, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    }

    reportError("Unterminated string literal");
    stateStack.pop();
    return Token(TokenType::ERROR, currentString, startLine, startColumn, filename);
}

Token Lexer::processCharLiteral() {
    int startLine = tokenStart.line;
    int startColumn = tokenStart.column;
    std::string charValue;

    while (currentChar != '\0') {

        if (currentChar == '\n' || currentChar == '\r') {
            reportError("Unterminated character literal");
            stateStack.pop();
            return Token(TokenType::ERROR, charValue, startLine, startColumn, filename);
        }

        if (currentChar == config.stringConfig.escapeChar) {
            charValue += currentChar;
            advance();

            if (currentChar != '\0') {
                charValue += currentChar;
                hasEscapeSequences = true;
                advance();
            }
        }
        else if (peekString(currentStringEnd.length()) == currentStringEnd) {
            for (size_t i = 0; i < currentStringEnd.length(); ++i) {
                advance();
            }

            Token token(TokenType::CHAR_LITERAL, charValue, startLine, startColumn, filename);

            if (hasEscapeSequences) {
                auto attribute = std::make_shared<StringAttribute>(false, true);
                token.attribute = attribute;
            }

            stateStack.pop();
            return token;
        }
        else {
            charValue += currentChar;
            advance();

            if (currentChar == '\0') {
                reportError("Unterminated character literal");
                Token token(TokenType::ERROR, charValue, startLine, startColumn, filename);
                stateStack.pop();
                return token;
            }
        }
    }

    reportError("Unterminated character literal");
    stateStack.pop();
    return Token(TokenType::ERROR, charValue, startLine, startColumn, filename);
}

Token Lexer::processOperator() {
    std::string op;
    int startLine = line;
    int startColumn = column;

    static const std::vector<std::string> common = {
        "<<=", ">>=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "==", "!=", "<=", ">="
    };
    auto matchOperators = [&](const auto& operators) {
      for (const auto& candidate : operators) {
        if (candidate == "?." && position + 2 < source.size() &&
            std::isdigit(static_cast<unsigned char>(source[position + 2]))) continue;
        if (candidate.size() > op.size() && source.compare(position, candidate.size(), candidate) == 0 &&
            std::all_of(candidate.begin(), candidate.end(), [&](char c) {
                return config.characterSets.isInSet(c, config.characterSets.operators) ||
                       config.characterSets.isInSet(c, config.characterSets.delimiters);
            })) op = candidate;
      }
    };
    matchOperators(common);
    matchOperators(config.additionalOperators);
    if (op.empty()) op = std::string(1, currentChar);
    for (size_t i = 0; i < op.size(); ++i) advance();

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
    int startLine = tokenStart.line;
    int startColumn = tokenStart.column;

    while (currentChar == ' ' || currentChar == '\t' || currentChar == '\r') advance();

    while (currentChar != '\0' && std::isalpha(currentChar)) {
        directive += currentChar;
        advance();
    }

    while (currentChar == ' ' || currentChar == '\t' || currentChar == '\r') advance();

    std::string args;
    while (currentChar != '\0' && currentChar != '\n') {
        args += currentChar;
        advance();
    }

    std::string fullDirective = directive + " " + args;

    Token token(TokenType::PREPROCESSOR, fullDirective, startLine, startColumn, filename);
    stateStack.pop();
    return token;
}
