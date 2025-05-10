#include "Token.h"

std::string Token::toString() const {
    std::string result = typeToString() + ": '" + lexeme + "' at " + location.toString();
    
    if (attribute) {
        result += " " + attribute->toString();
    }
    
    return result;
}

std::string Token::typeToString() const {
    switch (type) {
        // Literals
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::HEX: return "HEX";
        case TokenType::OCTAL: return "OCTAL";
        case TokenType::BINARY: return "BINARY";
        case TokenType::SCIENTIFIC: return "SCIENTIFIC";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        
        // Identifiers and keywords
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::KEYWORD: return "KEYWORD";
        
        // Operators
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::ASSIGNMENT_OPERATOR: return "ASSIGNMENT_OPERATOR";
        case TokenType::ARITHMETIC_OPERATOR: return "ARITHMETIC_OPERATOR";
        case TokenType::LOGICAL_OPERATOR: return "LOGICAL_OPERATOR";
        case TokenType::BITWISE_OPERATOR: return "BITWISE_OPERATOR";
        case TokenType::COMPARISON_OPERATOR: return "COMPARISON_OPERATOR";
        
        // Delimiters
        case TokenType::DELIMITER: return "DELIMITER";
        case TokenType::PARENTHESIS: return "PARENTHESIS";
        case TokenType::BRACKET: return "BRACKET";
        case TokenType::BRACE: return "BRACE";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        
        // Preprocessor
        case TokenType::PREPROCESSOR: return "PREPROCESSOR";
        
        // Comments
        case TokenType::COMMENT: return "COMMENT";
        
        // Special
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::ERROR: return "ERROR";
        case TokenType::UNKNOWN: return "UNKNOWN";
        
        default: return "UNDEFINED";
    }
}

// NumberAttribute implementation
std::string NumberAttribute::toString() const {
    std::string result = "[";
    
    switch (base) {
        case Base::DECIMAL: result += "decimal"; break;
        case Base::HEX: result += "hex"; break;
        case Base::OCTAL: result += "octal"; break;
        case Base::BINARY: result += "binary"; break;
    }
    
    if (isFloat) {
        result += ", float";
    }
    
    if (isScientific) {
        result += ", scientific";
    }
    
    result += "]";
    return result;
}

// StringAttribute implementation
std::string StringAttribute::toString() const {
    std::string result = "[";
    
    if (isRaw) {
        result += "raw";
    }
    
    if (isRaw && hasEscapeSequences) {
        result += ", ";
    }
    
    if (hasEscapeSequences) {
        result += "has escape sequences";
    }
    
    if (result == "[") {
        result += "regular string";
    }
    
    result += "]";
    return result;
}

// CommentAttribute implementation
std::string CommentAttribute::toString() const {
    std::string result = "[";
    
    switch (commentType) {
        case Type::SINGLE_LINE: result += "single-line"; break;
        case Type::MULTI_LINE: result += "multi-line"; break;
        case Type::DOC_COMMENT: result += "documentation"; break;
    }
    
    result += "]";
    return result;
}

// IdentifierAttribute implementation
std::string IdentifierAttribute::toString() const {
    std::string result = "[";
    
    if (isDeclared) {
        result += "declared";
        
        if (isDefinition) {
            result += ", definition";
        }
    } else {
        result += "undeclared";
    }
    
    if (!scope.empty()) {
        if (result != "[") {
            result += ", ";
        }
        result += "scope: " + scope;
    }
    
    result += "]";
    return result;
}

// PreprocessorAttribute implementation
std::string PreprocessorAttribute::toString() const {
    std::string result = "[";
    
    switch (directiveType) {
        case Type::INCLUDE: result += "include"; break;
        case Type::DEFINE: result += "define"; break;
        case Type::UNDEF: result += "undef"; break;
        case Type::IF: result += "if"; break;
        case Type::IFDEF: result += "ifdef"; break;
        case Type::IFNDEF: result += "ifndef"; break;
        case Type::ELSE: result += "else"; break;
        case Type::ELIF: result += "elif"; break;
        case Type::ENDIF: result += "endif"; break;
        case Type::PRAGMA: result += "pragma"; break;
        case Type::ERROR: result += "error"; break;
        case Type::WARNING: result += "warning"; break;
    }
    
    result += "]";
    return result;
} 