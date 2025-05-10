#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "Lexer.h"
#include "LanguageConfig.h"
#include "SymbolTable.h"
#include "ExportFormatter.h"

// Utility functions
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void processString(const std::string& source, const LanguageConfig& config, bool verbose = false) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create lexer with configuration
    Lexer lexer(source, config);
    
    // Create symbol table
    auto symbolTable = std::make_shared<SymbolTable>();
    lexer.setSymbolTable(symbolTable);
    
    // Get tokens
    std::vector<Token> tokens = lexer.tokenize();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Display results
    std::cout << "Lexical Analysis Results:" << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    std::cout << "Processing time: " << duration << " ms" << std::endl;
    
    if (lexer.hasErrors()) {
        std::cout << "\nErrors found during lexical analysis:" << std::endl;
        std::cout << lexer.getErrorReport();
    } else {
        std::cout << "No lexical errors detected." << std::endl;
    }
    
    if (verbose) {
        std::cout << "\nTokens:" << std::endl;
        for (const auto& token : tokens) {
            std::cout << token.toString() << std::endl;
        }
        
        // Show symbol table
        std::cout << "\nSymbol Table:" << std::endl;
        std::cout << symbolTable->toString() << std::endl;
    }
}

void processFile(const std::string& filename, const std::string& language, bool verbose = false, 
                const std::string& exportFormat = "", const std::string& exportFile = "") {
    std::string source = readFile(filename);
    if (source.empty()) {
        return;
    }
    
    // Determine language and create configuration
    LanguageConfig config;
    if (language == "c") {
        config = LanguageConfig::createCConfig();
    } else if (language == "cpp" || language == "c++") {
        config = LanguageConfig::createCppConfig();
    } else if (language == "java") {
        config = LanguageConfig::createJavaConfig();
    } else if (language == "python" || language == "py") {
        config = LanguageConfig::createPythonConfig();
    } else if (language == "javascript" || language == "js") {
        config = LanguageConfig::createJavaScriptConfig();
    } else {
        // Default to C++ if language not specified or recognized
        config = LanguageConfig::createCppConfig();
    }
    
    std::cout << "Processing file: " << filename << " (Language: " << config.getName() << ")" << std::endl;
    
    // Create lexer with configuration
    Lexer lexer(source, config, filename);
    
    // Create symbol table
    auto symbolTable = std::make_shared<SymbolTable>();
    lexer.setSymbolTable(symbolTable);
    
    // Get tokens
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<Token> tokens = lexer.tokenize();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Display results
    std::cout << "Lexical Analysis Results:" << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    std::cout << "Processing time: " << duration << " ms" << std::endl;
    
    if (lexer.hasErrors()) {
        std::cout << "\nErrors found during lexical analysis:" << std::endl;
        std::cout << lexer.getErrorReport();
    } else {
        std::cout << "No lexical errors detected." << std::endl;
    }
    
    if (verbose) {
        std::cout << "\nTokens:" << std::endl;
        for (const auto& token : tokens) {
            std::cout << token.toString() << std::endl;
        }
        
        // Show symbol table
        std::cout << "\nSymbol Table:" << std::endl;
        std::cout << symbolTable->toString() << std::endl;
    }
    
    // Export tokens if requested
    if (!exportFormat.empty() && !exportFile.empty()) {
        ExporterFactory::Format format;
        
        if (exportFormat == "json") {
            format = ExporterFactory::Format::JSON;
        } else if (exportFormat == "xml") {
            format = ExporterFactory::Format::XML;
        } else if (exportFormat == "csv") {
            format = ExporterFactory::Format::CSV;
        } else if (exportFormat == "html") {
            format = ExporterFactory::Format::HTML;
        } else {
            std::cerr << "Unsupported export format: " << exportFormat << std::endl;
            return;
        }
        
        auto exporter = ExporterFactory::createExporter(format);
        if (exporter) {
            bool success = exporter->exportToFile(tokens, exportFile);
            if (success) {
                std::cout << "Tokens exported to " << exportFile << " in " << exportFormat << " format." << std::endl;
            } else {
                std::cerr << "Failed to export tokens to " << exportFile << std::endl;
            }
        }
    }
}

void interactiveMode(const std::string& language = "cpp") {
    std::string line;
    
    // Determine language and create configuration
    LanguageConfig config;
    if (language == "c") {
        config = LanguageConfig::createCConfig();
    } else if (language == "cpp" || language == "c++") {
        config = LanguageConfig::createCppConfig();
    } else if (language == "java") {
        config = LanguageConfig::createJavaConfig();
    } else if (language == "python" || language == "py") {
        config = LanguageConfig::createPythonConfig();
    } else if (language == "javascript" || language == "js") {
        config = LanguageConfig::createJavaScriptConfig();
    } else {
        // Default to C++ if language not specified or recognized
        config = LanguageConfig::createCppConfig();
    }
    
    std::cout << "Lex Interactive Mode (Language: " << config.getName() << ")" << std::endl;
    std::cout << "Enter 'exit' to quit, 'language <name>' to change language" << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line == "exit") {
            break;
        }
        
        // Check for language change command
        if (line.substr(0, 9) == "language ") {
            std::string newLang = line.substr(9);
            
            if (newLang == "c") {
                config = LanguageConfig::createCConfig();
            } else if (newLang == "cpp" || newLang == "c++") {
                config = LanguageConfig::createCppConfig();
            } else if (newLang == "java") {
                config = LanguageConfig::createJavaConfig();
            } else if (newLang == "python" || newLang == "py") {
                config = LanguageConfig::createPythonConfig();
            } else if (newLang == "javascript" || newLang == "js") {
                config = LanguageConfig::createJavaScriptConfig();
            } else {
                std::cout << "Unsupported language: " << newLang << std::endl;
                continue;
            }
            
            std::cout << "Language changed to: " << config.getName() << std::endl;
            continue;
        }
        
        processString(line, config, true);
    }
}

void printUsage() {
    std::cout << "Usage: lex [options] [file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --interactive              Start in interactive mode" << std::endl;
    std::cout << "  -l, --language <lang>          Specify language (c, cpp, java, python, js)" << std::endl;
    std::cout << "  -v, --verbose                  Show detailed token information" << std::endl;
    std::cout << "  -e, --export <format>          Export tokens in format (json, xml, csv, html)" << std::endl;
    std::cout << "  -o, --output <file>            Output file for export" << std::endl;
    std::cout << "  -h, --help                     Display this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        interactiveMode();
        return 0;
    }
    
    // Process command line arguments
    std::string filename;
    std::string language = "cpp"; // Default language
    bool verbose = false;
    std::string exportFormat;
    std::string exportFile;
    bool interactive = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "-i" || arg == "--interactive") {
            interactive = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-l" || arg == "--language") {
            if (i + 1 < argc) {
                language = argv[++i];
            } else {
                std::cerr << "Error: --language requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-e" || arg == "--export") {
            if (i + 1 < argc) {
                exportFormat = argv[++i];
            } else {
                std::cerr << "Error: --export requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                exportFile = argv[++i];
            } else {
                std::cerr << "Error: --output requires an argument" << std::endl;
                return 1;
            }
        } else if (arg[0] != '-') {
            filename = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    if (interactive) {
        interactiveMode(language);
        return 0;
    }
    
    if (!filename.empty()) {
        processFile(filename, language, verbose, exportFormat, exportFile);
    } else {
        std::cerr << "No input file specified" << std::endl;
        printUsage();
        return 1;
    }
    
    return 0;
} 