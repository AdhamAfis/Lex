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
#include "ConfigLoader.h"
#include "LanguagePlugin.h"

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

// Helper function to get language configuration
LanguageConfig getLanguageConfig(const std::string& language, const std::string& configFile = "") {
    // First check if a custom config file is provided
    if (!configFile.empty()) {
        try {
            LanguageConfig config = ConfigLoader::loadLanguageFromFile(configFile);
            std::cout << "Using custom language configuration: " << config.getName() << " " << config.getVersion() << std::endl;
            return config;
        } catch (const std::exception& e) {
            std::cerr << "Error loading configuration file: " << e.what() << std::endl;
            std::cerr << "Falling back to plugins." << std::endl;
        }
    }
    
    // Now exclusively use plugins for all language configurations
    auto& pluginManager = LanguagePluginManager::getInstance();
    
    // Handle some common aliases
    std::string langKey = language;
    if (language == "c++") langKey = "cpp";
    if (language == "py") langKey = "python";
    if (language == "javascript") langKey = "js";
    
    if (pluginManager.hasLanguage(langKey)) {
        try {
            return pluginManager.loadLanguage(langKey);
        } catch (const std::exception& e) {
            std::cerr << "Error loading language plugin: " << e.what() << std::endl;
        }
    }
    
    // If language not found, default to c++ plugin
    std::cerr << "Unrecognized language '" << language << "', defaulting to C++" << std::endl;
    try {
        return pluginManager.loadLanguage("cpp");
    } catch (const std::exception& e) {
        std::cerr << "Error loading default cpp plugin: " << e.what() << std::endl;
        throw std::runtime_error("Could not load any language configuration");
    }
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
                const std::string& exportFormat = "", const std::string& exportFile = "",
                const std::string& configFile = "") {
    std::string source = readFile(filename);
    if (source.empty()) {
        return;
    }
    
    // Get language configuration
    LanguageConfig config = getLanguageConfig(language, configFile);
    
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

void interactiveMode(const std::string& language = "cpp", const std::string& configFile = "") {
    std::string line;
    
    // Get language configuration
    LanguageConfig config = getLanguageConfig(language, configFile);
    
    std::cout << "Lex Interactive Mode (Language: " << config.getName() << ")" << std::endl;
    std::cout << "Enter 'exit' to quit, 'language <name>' to change language," << std::endl;
    std::cout << "'export-config <file>' to export current language config to a file," << std::endl;
    std::cout << "'load-config <file>' to load a language config from a file," << std::endl;
    std::cout << "'list-plugins' to show available language plugins" << std::endl;
    
    auto& pluginManager = LanguagePluginManager::getInstance();
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line == "exit") {
            break;
        }
        
        // Check for language change command
        if (line.substr(0, 9) == "language ") {
            std::string newLang = line.substr(9);
            
            try {
                config = getLanguageConfig(newLang);
                std::cout << "Language changed to: " << config.getName() << " " << config.getVersion() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error changing language: " << e.what() << std::endl;
            }
            continue;
        }
        
        // Check for export config command
        if (line.substr(0, 14) == "export-config ") {
            std::string filename = line.substr(14);
            bool success = ConfigLoader::saveLanguageToFile(config, filename);
            if (success) {
                std::cout << "Language configuration exported to: " << filename << std::endl;
            } else {
                std::cerr << "Failed to export language configuration to file" << std::endl;
            }
            continue;
        }
        
        // Check for load config command
        if (line.substr(0, 12) == "load-config ") {
            std::string filename = line.substr(12);
            try {
                config = ConfigLoader::loadLanguageFromFile(filename);
                std::cout << "Loaded language configuration: " << config.getName() << " " << config.getVersion() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error loading configuration file: " << e.what() << std::endl;
            }
            continue;
        }
        
        // Check for list plugins command
        if (line == "list-plugins") {
            auto languages = pluginManager.getAvailableLanguages();
            
            if (languages.empty()) {
                std::cout << "No language plugins found. Place JSON language configurations in the 'plugins' directory." << std::endl;
            } else {
                std::cout << "Available language plugins:" << std::endl;
                for (const auto& lang : languages) {
                    std::cout << "  - " << lang << std::endl;
                }
            }
            continue;
        }
        
        processString(line, config, true);
    }
}

void exportLanguageConfig(const std::string& language, const std::string& filename) {
    LanguageConfig config = getLanguageConfig(language);
    
    bool success = ConfigLoader::saveLanguageToFile(config, filename);
    if (success) {
        std::cout << "Language configuration for " << config.getName() << " exported to: " << filename << std::endl;
    } else {
        std::cerr << "Failed to export language configuration to file" << std::endl;
    }
}

void printUsage() {
    std::cout << "Usage: lex [options] [file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --interactive              Start in interactive mode" << std::endl;
    std::cout << "  -l, --language <lang>          Specify language from available plugins" << std::endl;
    std::cout << "  -c, --config <file>            Use custom language configuration file" << std::endl;
    std::cout << "  -p, --plugins-dir <dir>        Specify plugins directory (default: ./plugins)" << std::endl;
    std::cout << "  -v, --verbose                  Show detailed token information" << std::endl;
    std::cout << "  -e, --export <format>          Export tokens in format (json, xml, csv, html)" << std::endl;
    std::cout << "  -o, --output <file>            Output file for export" << std::endl;
    std::cout << "  --export-config <lang> <file>  Export language config to a JSON file" << std::endl;
    std::cout << "  --list-plugins                 List available language plugins" << std::endl;
    std::cout << "  -h, --help                     Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Available plugins are in the 'plugins' directory. Run with --list-plugins to see them." << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize the plugin manager and scan for language plugins
    auto& pluginManager = LanguagePluginManager::getInstance();
    pluginManager.scanForPlugins();
    
    // Check if any language plugins were loaded
    auto languages = pluginManager.getAvailableLanguages();
    if (languages.empty()) {
        std::cerr << "Error: No language plugins found." << std::endl;
        std::cerr << "Place language configuration files in the 'plugins' directory." << std::endl;
        return 1;
    }
    
    if (argc == 1) {
        interactiveMode();
        return 0;
    }
    
    // Process command line arguments
    std::string filename;
    std::string language = "c++"; // Default language (will be mapped to cpp)
    std::string configFile;
    std::string pluginsDir;
    bool verbose = false;
    std::string exportFormat;
    std::string exportFile;
    bool interactive = false;
    bool exportConfig = false;
    std::string exportConfigLang;
    std::string exportConfigFile;
    bool listPlugins = false;
    
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
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            } else {
                std::cerr << "Error: --config requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-p" || arg == "--plugins-dir") {
            if (i + 1 < argc) {
                pluginsDir = argv[++i];
                pluginManager.setPluginsDirectory(pluginsDir);
            } else {
                std::cerr << "Error: --plugins-dir requires an argument" << std::endl;
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
        } else if (arg == "--export-config") {
            if (i + 2 < argc) {
                exportConfig = true;
                exportConfigLang = argv[++i];
                exportConfigFile = argv[++i];
            } else {
                std::cerr << "Error: --export-config requires two arguments" << std::endl;
                return 1;
            }
        } else if (arg == "--list-plugins") {
            listPlugins = true;
        } else if (arg[0] != '-') {
            filename = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    if (listPlugins) {
        auto languages = pluginManager.getAvailableLanguages();
        
        if (languages.empty()) {
            std::cout << "No language plugins found. Place JSON language configurations in the 'plugins' directory." << std::endl;
        } else {
            std::cout << "Available language plugins:" << std::endl;
            for (const auto& lang : languages) {
                std::cout << "  - " << lang << std::endl;
            }
        }
        return 0;
    }
    
    if (exportConfig) {
        exportLanguageConfig(exportConfigLang, exportConfigFile);
        return 0;
    }
    
    if (interactive) {
        interactiveMode(language, configFile);
        return 0;
    }
    
    if (!filename.empty()) {
        // If no language specified, try to guess from file extension
        if (language.empty() || language == "c++") {
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string ext = filename.substr(dotPos + 1);
                if (ext == "c") language = "c";
                else if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "h") language = "c++";
                else if (ext == "java") language = "java";
                else if (ext == "py") language = "python";
                else if (ext == "js") language = "javascript";
            }
        }
        
        processFile(filename, language, verbose, exportFormat, exportFile, configFile);
    } else {
        std::cerr << "No input file specified" << std::endl;
        printUsage();
        return 1;
    }
    
    return 0;
} 