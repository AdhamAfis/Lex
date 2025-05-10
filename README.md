# Lex - A Simple Lexical Analyzer

This is a simple lexical analyzer (lexer) written in C++. It breaks down source code into tokens that can be used for further parsing or compilation.

## Features

- Identifies various token types: integers, floats, identifiers, keywords, operators, delimiters, and string literals
- Line and column tracking for each token
- Interactive mode for testing
- File processing capability

## Building

To build the project, simply run:

```
make
```

This will compile the source code and create the `lex` executable.

## Usage

### Interactive Mode

Run the lexer in interactive mode:

```
./lex
```

or

```
./lex -i
```

In this mode, you can type expressions and see the resulting tokens. Type `exit` to quit.

### Processing a File

To analyze a file:

```
./lex path/to/file
```

### Help

For help on using the program:

```
./lex -h
```

## Example

```
> int x = 5 + 10;
Tokens:
KEYWORD: 'int' at line 1, column 1
IDENTIFIER: 'x' at line 1, column 5
OPERATOR: '=' at line 1, column 7
INTEGER: '5' at line 1, column 9
OPERATOR: '+' at line 1, column 11
INTEGER: '10' at line 1, column 13
DELIMITER: ';' at line 1, column 15
EOF: '' at line 1, column 16
```

## License

This project is open source and available under the MIT License. 