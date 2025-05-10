# Lex - Advanced Lexical Analyzer

A powerful lexical analyzer (lexer) written in C++ with support for multiple programming languages and advanced features. This tool tokenizes source code into meaningful lexical elements such as keywords, identifiers, operators, and literals.

## Table of Contents

- [Lex - Advanced Lexical Analyzer](#lex---advanced-lexical-analyzer)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Building from Source](#building-from-source)
  - [Project Structure](#project-structure)
  - [Architecture](#architecture)
  - [Usage](#usage)
    - [Command Line Options](#command-line-options)
    - [Interactive Mode](#interactive-mode)
    - [Processing Files](#processing-files)
    - [Exporting Results](#exporting-results)
  - [Supported Languages](#supported-languages)
  - [Examples](#examples)
    - [Basic Tokenization](#basic-tokenization)
    - [Analyzing a File](#analyzing-a-file)
    - [HTML Export Example](#html-export-example)
  - [Advanced Usage](#advanced-usage)
    - [Custom Language Configuration](#custom-language-configuration)
    - [Manual Token Processing](#manual-token-processing)
  - [Troubleshooting](#troubleshooting)
    - [Common Issues](#common-issues)
    - [Debug Mode](#debug-mode)
  - [Extending the Lexer](#extending-the-lexer)
  - [Contributing](#contributing)
  - [License](#license)

## Features

- **Multiple Language Support**: Predefined configurations for C, C++, Java, Python, and JavaScript
- **Advanced Token Recognition**:
  - Regular expression based pattern matching
  - Complete number formats: decimal, hex, octal, binary, scientific notation
  - Support for multi-line and documentation comments
  - Unicode support
  - Preprocessor directives

- **Symbol Table Construction**: Tracks identifiers and their scopes
- **Export Capabilities**:
  - Export tokens to JSON, XML, CSV, and HTML formats
  - Visual token stream representation in HTML

- **Detailed Error Reporting**: Precise error location and context information
- **Performance Metrics**: Track lexing performance statistics

## Installation

### Prerequisites

- C++ compiler with C++17 support (GCC 7+, Clang 5+, or MSVC 2017+)
- Make build system (optional, but recommended)

### Building from Source

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/lex.git
   cd lex
   ```

2. Build using Make:
   ```
   make
   ```

3. Verify the installation:
   ```
   ./lex --help
   ```

## Project Structure

```
lex/
├── src/                 # Source code
│   ├── main.cpp         # Entry point
│   ├── Lexer.h/cpp      # Core lexer implementation
│   ├── Token.h/cpp      # Token definitions
│   ├── SymbolTable.h/cpp # Symbol table implementation
│   ├── LanguageConfig.h/cpp # Language configurations
│   └── ExportFormatter.h/cpp # Output formatting
├── tests/               # Test files
├── Makefile             # Build configuration
├── .gitignore           # Git ignore file
├── README.md            # This file
└── lex                  # Compiled executable
```

## Architecture

The lexer is designed with the following components:

1. **Lexer**: The core component that reads source code and produces tokens
2. **TokenStream**: A sequence of tokens that can be traversed
3. **SymbolTable**: Maintains a database of identifiers and their scopes
4. **LanguageConfig**: Defines language-specific rules and patterns
5. **ExportFormatter**: Formats token output in various formats

## Usage

### Command Line Options

```
Usage: lex [options] [file]
Options:
  -i, --interactive              Start in interactive mode
  -l, --language <lang>          Specify language (c, cpp, java, python, js)
  -v, --verbose                  Show detailed token information
  -e, --export <format>          Export tokens in format (json, xml, csv, html)
  -o, --output <file>            Output file for export
  -h, --help                     Display this help message
```

### Interactive Mode

Run the lexer in interactive mode:

```
./lex -i
```

or with a specific language:

```
./lex -i -l python
```

In this mode, you can type expressions and see the resulting tokens. Type `exit` to quit or `language <n>` to change the active language.

### Processing Files

To analyze a file:

```
./lex path/to/file
```

With language specification:

```
./lex -l java path/to/file.java
```

With verbose output:

```
./lex -v path/to/file
```

### Exporting Results

Export tokens to various formats:

```
./lex -l cpp -e json -o tokens.json path/to/file.cpp
```

Available export formats:
- `json`: Structured JSON format
- `xml`: XML document
- `csv`: Comma-separated values
- `html`: Interactive HTML visualization

## Supported Languages

Lex comes with built-in support for:

| Language      | Flag           | File Extensions |
|---------------|----------------|-----------------|
| C             | `-l c`         | .c, .h          |
| C++           | `-l cpp`       | .cpp, .hpp, .cc |
| Java          | `-l java`      | .java           |
| Python        | `-l python`    | .py             |
| JavaScript    | `-l js`        | .js             |

## Examples

### Basic Tokenization

```
$ ./lex -i -l cpp
Lex Interactive Mode (Language: C++)
Enter 'exit' to quit, 'language <n>' to change language
> int main() { return 0; }
Lexical Analysis Results:
------------------------
Total tokens: 9
Processing time: 1 ms
No lexical errors detected.

Tokens:
KEYWORD: 'int' at :1:1
IDENTIFIER: 'main' at :1:5 [undeclared]
PARENTHESIS: '(' at :1:9
PARENTHESIS: ')' at :1:10
BRACE: '{' at :1:12
KEYWORD: 'return' at :1:14
INTEGER: '0' at :1:21 [decimal]
SEMICOLON: ';' at :1:22
BRACE: '}' at :1:24
EOF: '' at :1:26

Symbol Table:
Symbol Table: 1 unique symbols
Scope: global (Global) at :0:0 [1 symbols]
```

### Analyzing a File

```
$ ./lex -v path/to/file.cpp
Processing file: path/to/file.cpp (Language: C++)
Lexical Analysis Results:
------------------------
Total tokens: 145
Processing time: 12 ms
No lexical errors detected.

Tokens:
[... token listing ...]
```

### HTML Export Example

```
$ ./lex -l cpp -e html -o tokens.html path/to/file.cpp
Tokens exported to tokens.html in html format.
```

The HTML output provides a visual representation of the token stream with syntax highlighting.

## Advanced Usage

### Custom Language Configuration

The lexer can be extended with custom language configurations:

```cpp
// In your code
LanguageConfig customConfig;
customConfig.setName("MyLanguage");
customConfig.addKeywords({"if", "else", "while", "for"});
// ... more configuration

Lexer lexer(source, customConfig);
```

### Manual Token Processing

```cpp
// In your code
Lexer lexer(source);
std::vector<Token> tokens = lexer.tokenize();

// Process tokens manually
for (const auto& token : tokens) {
    // Do something with each token
}
```

## Troubleshooting

### Common Issues

1. **Compilation errors**
   - Make sure you have a C++17 compatible compiler
   - Check that all dependencies are installed

2. **Language detection issues**
   - Explicitly specify the language with the `-l` flag
   - Check that your file extension is recognized

3. **Export failures**
   - Ensure the output directory exists and is writable
   - Check that the specified format is supported

### Debug Mode

For detailed debug output, compile with:

```
make debug
```

## Extending the Lexer

The lexer can be easily extended with:

1. **New Language Support**: 
   Add new language configurations in `LanguageConfig.cpp`

2. **Additional Token Types**: 
   Extend the `TokenType` enum in `Token.h`

3. **Custom Export Formats**: 
   Create new exporters in `ExportFormatter.cpp`

4. **Adding New Features**:
   - Implement additional symbol table functionality
   - Add more advanced error recovery mechanisms
   - Extend token attribute handling

## Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure your code follows the project's coding style and includes appropriate tests.

## License

This project is open source and available under the MIT License.
