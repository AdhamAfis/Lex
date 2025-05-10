#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <vector>
#include <memory>

// Source location information
struct SourceLocation {
    int line;
    int column;
    std::string filename;
    
    SourceLocation(int line = 0, int column = 0, const std::string& filename = "")
        : line(line), column(column), filename(filename) {}
    
    std::string toString() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

// Expanded token types
enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    HEX,
    OCTAL,
    BINARY,
    SCIENTIFIC,
    STRING_LITERAL,
    CHAR_LITERAL,
    
    // Identifiers and keywords
    IDENTIFIER,
    KEYWORD,
    
    // Operators
    OPERATOR,
    ASSIGNMENT_OPERATOR,
    ARITHMETIC_OPERATOR, 
    LOGICAL_OPERATOR,
    BITWISE_OPERATOR,
    COMPARISON_OPERATOR,
    
    // Delimiters
    DELIMITER,
    PARENTHESIS,
    BRACKET,
    BRACE,
    SEMICOLON,
    COMMA,
    DOT,
    
    // Preprocessor
    PREPROCESSOR,
    
    // Comments
    COMMENT,
    
    // Special
    EOF_TOKEN,
    ERROR,
    UNKNOWN
};

// Forward declaration for token attribute
class TokenAttribute;

// Main token class
class Token {
public:
    TokenType type;
    std::string lexeme;
    SourceLocation location;
    std::shared_ptr<TokenAttribute> attribute;
    
    Token(TokenType type, const std::string& lexeme, int line, int column, const std::string& filename = "")
        : type(type), lexeme(lexeme), location(line, column, filename) {}
    
    Token(TokenType type, const std::string& lexeme, const SourceLocation& location)
        : type(type), lexeme(lexeme), location(location) {}
        
    std::string toString() const;
    std::string typeToString() const;
};

// Base class for specialized token attributes
class TokenAttribute {
public:
    virtual ~TokenAttribute() = default;
    virtual std::string toString() const = 0;
};

// Number token attributes
class NumberAttribute : public TokenAttribute {
public:
    enum class Base { DECIMAL, HEX, OCTAL, BINARY };
    Base base;
    bool isFloat;
    bool isScientific;
    
    NumberAttribute(Base base = Base::DECIMAL, bool isFloat = false, bool isScientific = false)
        : base(base), isFloat(isFloat), isScientific(isScientific) {}
        
    std::string toString() const override;
};

// String token attributes
class StringAttribute : public TokenAttribute {
public:
    bool isRaw;
    bool hasEscapeSequences;
    
    StringAttribute(bool isRaw = false, bool hasEscapeSequences = false)
        : isRaw(isRaw), hasEscapeSequences(hasEscapeSequences) {}
        
    std::string toString() const override;
};

// Comment token attributes
class CommentAttribute : public TokenAttribute {
public:
    enum class Type { SINGLE_LINE, MULTI_LINE, DOC_COMMENT };
    Type commentType;
    
    CommentAttribute(Type commentType = Type::SINGLE_LINE)
        : commentType(commentType) {}
        
    std::string toString() const override;
};

// Keyword or identifier token attributes
class IdentifierAttribute : public TokenAttribute {
public:
    bool isDeclared;
    bool isDefinition;
    std::string scope;
    
    IdentifierAttribute(bool isDeclared = false, bool isDefinition = false, const std::string& scope = "")
        : isDeclared(isDeclared), isDefinition(isDefinition), scope(scope) {}
        
    std::string toString() const override;
};

// Preprocessor token attributes
class PreprocessorAttribute : public TokenAttribute {
public:
    enum class Type { INCLUDE, DEFINE, UNDEF, IF, IFDEF, IFNDEF, ELSE, ELIF, ENDIF, PRAGMA, ERROR, WARNING };
    Type directiveType;
    
    PreprocessorAttribute(Type directiveType = Type::INCLUDE)
        : directiveType(directiveType) {}
        
    std::string toString() const override;
};

#endif // TOKEN_H 