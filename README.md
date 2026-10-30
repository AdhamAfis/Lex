# Lex

Lex is a C++17 lexical analyzer that turns source code into tokens such as keywords, identifiers, literals, and operators. It includes JSON language configurations for C, C++, Java, Python, and JavaScript, a command-line interface, and a browser interface built with WebAssembly.

This is a learning project for exploring lexer design and configurable token rules. The language configurations cover common syntax; they do not implement the full specifications of those languages.

[Browser demo](https://adhamafis.github.io/lex/) · [Build](#build-and-run) · [Design](#design) · [Limitations](#limitations)

## Build and run

You need a C++17 compiler and Make. The test suite also requires Python 3 and Node.js 20 or newer.

```sh
git clone https://github.com/AdhamAfis/Lex.git
cd Lex
make
./lex -v tests/test.js
```

Lex loads language configurations from `./plugins` by default, so run it from the repository root or provide a plugin directory with `-p`.

The sample JavaScript file includes `function test() { return 42; }`. Its token output includes:

```text
KEYWORD: 'function' at tests/test.js:1:1
IDENTIFIER: 'test' at tests/test.js:1:10 [undeclared, scope: global]
PARENTHESIS: '(' at tests/test.js:1:14
...
KEYWORD: 'return' at tests/test.js:1:19
INTEGER: '42' at tests/test.js:1:26
```

Use `-v` to print tokens and the symbol-table summary. Without it, Lex prints a summary and any lexical errors. For file processing, a nonzero exit status indicates a read, lexical, or export error. Exports replace regular files atomically; new export files are private to the current user.

## Usage

```sh
# Analyze a file; Lex can infer the language from common extensions
./lex -v path/to/source.cpp

# Select a language explicitly
./lex -v -l python path/to/source.py

# Export tokens to JSON, XML, CSV, or HTML
./lex -l cpp -e json -o tokens.json path/to/source.cpp

# Start interactive mode, list plugins, or see all options
./lex -i -l cpp
./lex --list-plugins
./lex --help
```

Interactive mode processes one line at a time. Enter `exit` to quit, `language python` to switch languages, or `list-plugins` to list available configurations.

| Language | Flag | Common extensions |
| --- | --- | --- |
| C | `-l c` | `.c` |
| C++ | `-l cpp` | `.cpp`, `.cc`, `.cxx`, `.hpp`, `.h` |
| Java | `-l java` | `.java` |
| Python | `-l python` | `.py` |
| JavaScript | `-l js` | `.js` |

## Language configurations

The CLI scans JSON files with a nonempty `name` in the plugins directory. Plugin names and filename aliases must be unique (case-insensitively); a collision prevents plugins from loading. Start with a bundled configuration when creating your own:

```sh
./lex --export-config cpp custom_cpp.json
./lex -c custom_cpp.json -v path/to/source.cpp
```

Alternatively, place a configuration in a plugin directory and select it by name:

```sh
./lex -p /path/to/plugins --list-plugins
./lex -p /path/to/plugins -l mylanguage path/to/source.txt
```

A configuration can define keywords, character sets, comment and string delimiters, and regular-expression token rules. See the files in [`plugins/`](plugins/) and the token types in [`src/Token.h`](src/Token.h). Token-rule types are stored as numeric enum values; EOF, ERROR, and UNKNOWN are reserved and cannot be used in rules. Rule order matters because Lex uses the first matching rule. Test changes against representative source before relying on a configuration.

Number patterns belong in `tokenRules`. Older `numberConfig` fields were unused and are ignored when loading older configurations. Token-rule patterns use standard JSON string escaping. Configurations exported by older versions may have extra backslashes in their patterns and need those removed before use.
Custom C or C++ configurations exported by older versions also need `"preprocessorConfig": {"directiveStart": "#"}` to recognize directives.
Language-specific multi-character operators are listed in `additionalOperators`. C++ raw-string handling uses `stringConfig.cppRawStrings`; copy these settings from the bundled configurations when updating older custom files.

If a custom configuration cannot be loaded, the CLI reports the error and exits. An unrecognized language falls back to the C++ plugin.

## Design

Lex separates language configuration from token processing so that the same lexer can be used with different JSON definitions. The main flow is:

1. `LanguagePluginManager` and `ConfigLoader` load a language configuration.
2. `Lexer` scans the input, handles comments and strings, tries configured token rules, and returns tokens with source locations.
3. `SymbolTable` records encountered identifiers. The CLI prints results or passes tokens to an `ExportFormatter`.
4. The WebAssembly bindings expose the lexer to the browser interface in `web/`.

This design makes language rules easy to inspect and change without rebuilding the native program. It also means that a general-purpose scanner and configuration must approximate language-specific rules that a dedicated compiler lexer would handle in more detail.

## Limitations

- Language configurations are partial. Passing analysis does not establish that a program is syntactically or semantically valid.
- The symbol table records encountered identifiers, but Lex does not parse declarations or provide complete scope or type analysis. An identifier shown as undeclared is not a compiler diagnostic.
- Lex recognizes preprocessor directives as tokens; it does not perform full preprocessing or macro expansion.
- Unicode and language-specific edge cases have not been comprehensively implemented or tested. Rule order, overlapping patterns, number formats, strings, and comments deserve particular care when editing plugins.
- Python formatted strings are returned as single string tokens; expressions inside them are not parsed or tokenized separately.
- The web interface analyzes entered text and displays tokens. It does not expose every CLI option or export format.

## Tests

```sh
make test
```

The regression suite checks representative token types across the bundled configurations, incomplete strings and comments, directive boundaries, token locations, JSON export, CLI failures, plugin-directory selection, interactive input closure, and browser response-buffer ownership. These are focused regression checks, not exhaustive language-conformance tests. GitHub Actions is configured to run them on pushes and pull requests.


## Browser build

Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html), activate it, then run:

```sh
make -f emscripten.mk
python3 -m http.server 8000 --directory web
```

Open [localhost:8000](http://localhost:8000). The build generates `web/lex.js`, `web/lex.wasm`, and `web/lex.data`; the language plugins are preloaded into the WebAssembly filesystem. The repository includes a GitHub Pages deployment workflow.

To run the WebAssembly regression check in Node.js, use `make -f emscripten.mk test`.

## Project layout

- [`src/`](src/) — lexer, token and symbol types, configuration, exporters, CLI, and WebAssembly bindings
- [`plugins/`](plugins/) — bundled JSON language configurations
- [`web/`](web/) — browser interface
- [`tests/`](tests/) — sample inputs and regression tests
- [`Makefile`](Makefile) and [`emscripten.mk`](emscripten.mk) — native and WebAssembly builds

Lex uses the bundled [nlohmann/json](https://github.com/nlohmann/json) library for JSON parsing and output. Lex and that library are distributed under the MIT license; see [LICENSE](LICENSE).
