#include "LanguageConfig.h"

LanguageConfig::LanguageConfig(const std::string& name, const std::string& version)
    : name(name), version(version) {}

void LanguageConfig::addTokenRule(const TokenRule& rule) {
    tokenRules.push_back(rule);
}

void LanguageConfig::addKeyword(const std::string& keyword) {
    keywordSets.keywords.insert(keyword);
}

void LanguageConfig::addType(const std::string& type) {
    keywordSets.types.insert(type);
}

void LanguageConfig::addOperator(const std::string& op) {
    keywordSets.operators.insert(op);
}

void LanguageConfig::addBuiltin(const std::string& builtin) {
    keywordSets.builtins.insert(builtin);
}

// Factory method to create a C language configuration
LanguageConfig LanguageConfig::createCConfig() {
    LanguageConfig config("C", "C99/C11");
    
    // Configure character sets
    config.characterSets.identifierStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    config.characterSets.identifierContinue = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    config.characterSets.operators = "+-*/%=<>!&|^~?:";
    config.characterSets.delimiters = "()[]{},;.";
    config.characterSets.whitespace = " \t\n\r\f\v";
    
    // Keywords
    const std::vector<std::string> keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", 
        "double", "else", "enum", "extern", "float", "for", "goto", "if", 
        "int", "long", "register", "return", "short", "signed", "sizeof", "static", 
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    
    for (const auto& keyword : keywords) {
        config.addKeyword(keyword);
    }
    
    // Types
    const std::vector<std::string> types = {
        "int", "char", "short", "long", "float", "double", "void", "signed", "unsigned"
    };
    
    for (const auto& type : types) {
        config.addType(type);
    }
    
    // Configure comments
    config.commentConfig.singleLineCommentStarts = {"//", "#"};
    config.commentConfig.multiLineCommentDelimiters = {{"/*", "*/"}};
    
    // Configure strings
    config.stringConfig.stringDelimiters = {{R"(")", R"(")"}};
    config.stringConfig.charDelimiters = {{"'", "'"}};
    config.stringConfig.escapeChar = '\\';
    
    // Configure numbers
    config.numberConfig.decimalIntPattern = R"(\b[0-9]+\b)";
    config.numberConfig.floatingPointPattern = R"(\b[0-9]+\.[0-9]+\b)";
    config.numberConfig.hexPattern = R"(\b0[xX][0-9a-fA-F]+\b)";
    config.numberConfig.octalPattern = R"(\b0[0-7]+\b)";
    config.numberConfig.scientificPattern = R"(\b[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+\b)";
    
    // Configure preprocessor
    config.preprocessorConfig.directivePrefix = "#";
    config.preprocessorConfig.directiveTypes = {
        {"include", PreprocessorAttribute::Type::INCLUDE},
        {"define", PreprocessorAttribute::Type::DEFINE},
        {"undef", PreprocessorAttribute::Type::UNDEF},
        {"if", PreprocessorAttribute::Type::IF},
        {"ifdef", PreprocessorAttribute::Type::IFDEF},
        {"ifndef", PreprocessorAttribute::Type::IFNDEF},
        {"else", PreprocessorAttribute::Type::ELSE},
        {"elif", PreprocessorAttribute::Type::ELIF},
        {"endif", PreprocessorAttribute::Type::ENDIF},
        {"pragma", PreprocessorAttribute::Type::PRAGMA},
        {"error", PreprocessorAttribute::Type::ERROR},
        {"warning", PreprocessorAttribute::Type::WARNING}
    };
    
    // Add token rules with regex patterns
    config.addTokenRule(TokenRule("Identifier", R"([a-zA-Z_][a-zA-Z0-9_]*)", TokenType::IDENTIFIER, 1));
    config.addTokenRule(TokenRule("HexNumber", R"(0[xX][0-9a-fA-F]+)", TokenType::HEX, 2));
    config.addTokenRule(TokenRule("OctalNumber", R"(0[0-7]+)", TokenType::OCTAL, 2));
    config.addTokenRule(TokenRule("ScientificNumber", R"([0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+)", TokenType::SCIENTIFIC, 2));
    config.addTokenRule(TokenRule("FloatNumber", R"([0-9]+\.[0-9]+)", TokenType::FLOAT, 2));
    config.addTokenRule(TokenRule("IntNumber", R"([0-9]+)", TokenType::INTEGER, 2));
    
    return config;
}

// Factory method to create a C++ language configuration
LanguageConfig LanguageConfig::createCppConfig() {
    LanguageConfig config = createCConfig();
    config.name = "C++";
    config.version = "C++17";
    
    // Additional C++ keywords
    const std::vector<std::string> additionalKeywords = {
        "asm", "bool", "catch", "class", "const_cast", "delete", "dynamic_cast",
        "explicit", "export", "false", "friend", "inline", "mutable", "namespace",
        "new", "operator", "private", "protected", "public", "reinterpret_cast",
        "static_cast", "template", "this", "throw", "true", "try", "typeid",
        "typename", "using", "virtual", "wchar_t", "alignas", "alignof", "constexpr",
        "decltype", "noexcept", "nullptr", "static_assert", "thread_local"
    };
    
    for (const auto& keyword : additionalKeywords) {
        config.addKeyword(keyword);
    }
    
    // Additional C++ types
    const std::vector<std::string> additionalTypes = {
        "bool", "wchar_t", "char16_t", "char32_t", "auto"
    };
    
    for (const auto& type : additionalTypes) {
        config.addType(type);
    }
    
    // C++ supports raw strings
    config.stringConfig.rawStringPrefix = "R";
    
    // Add C++ specific token rules
    config.addTokenRule(TokenRule("BinaryNumber", R"(0[bB][01]+)", TokenType::BINARY, 2));
    
    return config;
}

// Factory method to create a Java language configuration
LanguageConfig LanguageConfig::createJavaConfig() {
    LanguageConfig config("Java", "Java 11");
    
    // Configure character sets
    config.characterSets.identifierStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    config.characterSets.identifierContinue = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    config.characterSets.operators = "+-*/%=<>!&|^~?:";
    config.characterSets.delimiters = "()[]{},;.";
    config.characterSets.whitespace = " \t\n\r\f\v";
    
    // Keywords
    const std::vector<std::string> keywords = {
        "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
        "class", "const", "continue", "default", "do", "double", "else", "enum",
        "extends", "final", "finally", "float", "for", "goto", "if", "implements",
        "import", "instanceof", "int", "interface", "long", "native", "new", "package",
        "private", "protected", "public", "return", "short", "static", "strictfp", "super",
        "switch", "synchronized", "this", "throw", "throws", "transient", "try", "void",
        "volatile", "while", "var"
    };
    
    for (const auto& keyword : keywords) {
        config.addKeyword(keyword);
    }
    
    // Configure comments
    config.commentConfig.singleLineCommentStarts = {"//"};
    config.commentConfig.multiLineCommentDelimiters = {{"/*", "*/"}};
    config.commentConfig.docCommentStarts = {"/**"};
    
    // Configure strings
    config.stringConfig.stringDelimiters = {{R"(")", R"(")"}};
    config.stringConfig.charDelimiters = {{"'", "'"}};
    config.stringConfig.escapeChar = '\\';
    
    // Configure numbers similar to C++
    config.numberConfig.decimalIntPattern = R"(\b[0-9]+[lL]?\b)";
    config.numberConfig.floatingPointPattern = R"(\b[0-9]+\.[0-9]+[fFdD]?\b)";
    config.numberConfig.hexPattern = R"(\b0[xX][0-9a-fA-F]+[lL]?\b)";
    config.numberConfig.octalPattern = R"(\b0[0-7]+[lL]?\b)";
    config.numberConfig.binaryPattern = R"(\b0[bB][01]+[lL]?\b)";
    config.numberConfig.scientificPattern = R"(\b[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+[fFdD]?\b)";
    
    // Token rules
    config.addTokenRule(TokenRule("Identifier", R"([a-zA-Z_][a-zA-Z0-9_]*)", TokenType::IDENTIFIER, 1));
    config.addTokenRule(TokenRule("HexNumber", R"(0[xX][0-9a-fA-F]+[lL]?)", TokenType::HEX, 2));
    config.addTokenRule(TokenRule("BinaryNumber", R"(0[bB][01]+[lL]?)", TokenType::BINARY, 2));
    config.addTokenRule(TokenRule("OctalNumber", R"(0[0-7]+[lL]?)", TokenType::OCTAL, 2));
    config.addTokenRule(TokenRule("ScientificNumber", R"([0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+[fFdD]?)", TokenType::SCIENTIFIC, 2));
    config.addTokenRule(TokenRule("FloatNumber", R"([0-9]+\.[0-9]+[fFdD]?)", TokenType::FLOAT, 2));
    config.addTokenRule(TokenRule("IntNumber", R"([0-9]+[lL]?)", TokenType::INTEGER, 2));
    
    return config;
}

// Factory method to create a Python language configuration
LanguageConfig LanguageConfig::createPythonConfig() {
    LanguageConfig config("Python", "Python 3.9");
    
    // Configure character sets
    config.characterSets.identifierStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    config.characterSets.identifierContinue = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    config.characterSets.operators = "+-*/%=<>!&|^~@";
    config.characterSets.delimiters = "()[]{},;:.";
    config.characterSets.whitespace = " \t\n\r\f\v";
    
    // Keywords
    const std::vector<std::string> keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await", "break",
        "class", "continue", "def", "del", "elif", "else", "except", "finally",
        "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
        "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
    };
    
    for (const auto& keyword : keywords) {
        config.addKeyword(keyword);
    }
    
    // Built-ins
    const std::vector<std::string> builtins = {
        "abs", "all", "any", "bin", "bool", "bytearray", "bytes", "callable", "chr",
        "classmethod", "compile", "complex", "delattr", "dict", "dir", "divmod",
        "enumerate", "eval", "exec", "filter", "float", "format", "frozenset",
        "getattr", "globals", "hasattr", "hash", "help", "hex", "id", "input",
        "int", "isinstance", "issubclass", "iter", "len", "list", "locals", "map",
        "max", "memoryview", "min", "next", "object", "oct", "open", "ord", "pow",
        "print", "property", "range", "repr", "reversed", "round", "set", "setattr",
        "slice", "sorted", "staticmethod", "str", "sum", "super", "tuple", "type",
        "vars", "zip", "__import__"
    };
    
    for (const auto& builtin : builtins) {
        config.addBuiltin(builtin);
    }
    
    // Configure comments
    config.commentConfig.singleLineCommentStarts = {"#"};
    
    // Configure strings - Python has single, double and triple quoted strings
    config.stringConfig.stringDelimiters = {
        {R"(")", R"(")"},
        {R"(')", R"(')"},
        {R"(""")", R"(""")"},
        {R"(''')", R"(''')"}
    };
    config.stringConfig.escapeChar = '\\';
    config.stringConfig.rawStringPrefix = "r";
    
    // Configure numbers
    config.numberConfig.decimalIntPattern = R"(\b[0-9]+\b)";
    config.numberConfig.floatingPointPattern = R"(\b[0-9]+\.[0-9]*|[0-9]*\.[0-9]+\b)";
    config.numberConfig.hexPattern = R"(\b0[xX][0-9a-fA-F]+\b)";
    config.numberConfig.octalPattern = R"(\b0[oO][0-7]+\b)";
    config.numberConfig.binaryPattern = R"(\b0[bB][01]+\b)";
    config.numberConfig.scientificPattern = R"(\b[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+\b)";
    
    // Token rules
    config.addTokenRule(TokenRule("Identifier", R"([a-zA-Z_][a-zA-Z0-9_]*)", TokenType::IDENTIFIER, 1));
    config.addTokenRule(TokenRule("HexNumber", R"(0[xX][0-9a-fA-F]+)", TokenType::HEX, 2));
    config.addTokenRule(TokenRule("BinaryNumber", R"(0[bB][01]+)", TokenType::BINARY, 2));
    config.addTokenRule(TokenRule("OctalNumber", R"(0[oO][0-7]+)", TokenType::OCTAL, 2));
    config.addTokenRule(TokenRule("ScientificNumber", R"([0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+)", TokenType::SCIENTIFIC, 2));
    config.addTokenRule(TokenRule("FloatNumber", R"([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)", TokenType::FLOAT, 2));
    config.addTokenRule(TokenRule("IntNumber", R"([0-9]+)", TokenType::INTEGER, 2));
    
    return config;
}

// Factory method to create a JavaScript language configuration
LanguageConfig LanguageConfig::createJavaScriptConfig() {
    LanguageConfig config("JavaScript", "ECMAScript 2020");
    
    // Configure character sets
    config.characterSets.identifierStart = "_$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    config.characterSets.identifierContinue = "_$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    config.characterSets.operators = "+-*/%=<>!&|^~?:";
    config.characterSets.delimiters = "()[]{},;.";
    config.characterSets.whitespace = " \t\n\r\f\v";
    
    // Keywords
    const std::vector<std::string> keywords = {
        "break", "case", "catch", "class", "const", "continue", "debugger", "default",
        "delete", "do", "else", "export", "extends", "finally", "for", "function",
        "if", "import", "in", "instanceof", "new", "return", "super", "switch",
        "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
        "let", "static", "enum", "await", "implements", "package", "protected",
        "interface", "private", "public", "async", "null", "true", "false"
    };
    
    for (const auto& keyword : keywords) {
        config.addKeyword(keyword);
    }
    
    // Configure comments
    config.commentConfig.singleLineCommentStarts = {"//"};
    config.commentConfig.multiLineCommentDelimiters = {{"/*", "*/"}};
    
    // Configure strings - JavaScript has single, double and template strings
    config.stringConfig.stringDelimiters = {
        {R"(")", R"(")"},
        {R"(')", R"(')"},
        {R"(`)", R"(`)"}
    };
    config.stringConfig.escapeChar = '\\';
    
    // Configure numbers
    config.numberConfig.decimalIntPattern = R"(\b[0-9]+\b)";
    config.numberConfig.floatingPointPattern = R"(\b[0-9]+\.[0-9]*|[0-9]*\.[0-9]+\b)";
    config.numberConfig.hexPattern = R"(\b0[xX][0-9a-fA-F]+\b)";
    config.numberConfig.octalPattern = R"(\b0[oO][0-7]+\b)";
    config.numberConfig.binaryPattern = R"(\b0[bB][01]+\b)";
    config.numberConfig.scientificPattern = R"(\b[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+\b)";
    
    // Token rules
    config.addTokenRule(TokenRule("Identifier", R"([$_a-zA-Z][$_a-zA-Z0-9]*)", TokenType::IDENTIFIER, 1));
    config.addTokenRule(TokenRule("HexNumber", R"(0[xX][0-9a-fA-F]+)", TokenType::HEX, 2));
    config.addTokenRule(TokenRule("BinaryNumber", R"(0[bB][01]+)", TokenType::BINARY, 2));
    config.addTokenRule(TokenRule("OctalNumber", R"(0[oO][0-7]+)", TokenType::OCTAL, 2));
    config.addTokenRule(TokenRule("ScientificNumber", R"([0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+)", TokenType::SCIENTIFIC, 2));
    config.addTokenRule(TokenRule("FloatNumber", R"([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)", TokenType::FLOAT, 2));
    config.addTokenRule(TokenRule("IntNumber", R"([0-9]+)", TokenType::INTEGER, 2));
    
    return config;
} 