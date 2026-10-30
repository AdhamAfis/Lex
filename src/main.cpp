#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <filesystem>
#include "Lexer.h"
#include "LanguageConfig.h"
#include "SymbolTable.h"
#include "ExportFormatter.h"
#include "ConfigLoader.h"
#include "LanguagePlugin.h"
#include "FileUtil.h"

std::string readFile(const std::string& filename) {
    return readFileContents(filename);
}

LanguageConfig getLanguageConfig(const std::string& language, const std::string& configFile = "") {

    if (!configFile.empty()) {
        LanguageConfig config = ConfigLoader::loadLanguageFromFile(configFile);
        std::cout << "Using custom language configuration: " << config.getName() << " " << config.getVersion() << std::endl;
        return config;
    }

    auto& pluginManager = LanguagePluginManager::getInstance();

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

    Lexer lexer(source, config);

    auto symbolTable = std::make_shared<SymbolTable>();
    lexer.setSymbolTable(symbolTable);

    std::vector<Token> tokens = lexer.tokenize();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

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

        std::cout << "\nSymbol Table:" << std::endl;
        std::cout << symbolTable->toString() << std::endl;
    }
}

bool processFile(const std::string& filename, const std::string& language, bool verbose = false,
                const std::string& exportFormat = "", const std::string& exportFile = "",
                const std::string& configFile = "") {
    std::string source = readFile(filename);
    if (exportFormat.empty() != exportFile.empty()) {
        std::cerr << "Both --export and --output are required for export" << std::endl;
        return false;
    }
    if (!exportFile.empty() && std::filesystem::exists(exportFile) &&
        std::filesystem::equivalent(filename, exportFile)) {
        std::cerr << "Output file refers to the input file" << std::endl;
        return false;
    }

    if (!exportFile.empty() && !configFile.empty() && std::filesystem::exists(exportFile) &&
        std::filesystem::equivalent(configFile, exportFile)) {
        std::cerr << "Output file refers to the configuration file" << std::endl;
        return false;
    }

    LanguageConfig config = getLanguageConfig(language, configFile);

    std::cout << "Processing file: " << filename << " (Language: " << config.getName() << ")" << std::endl;

    Lexer lexer(source, config, filename);

    auto symbolTable = std::make_shared<SymbolTable>();
    lexer.setSymbolTable(symbolTable);

    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<Token> tokens = lexer.tokenize();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

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

        std::cout << "\nSymbol Table:" << std::endl;
        std::cout << symbolTable->toString() << std::endl;
    }

    if (lexer.hasErrors()) return false;

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
            return false;
        }

        auto exporter = ExporterFactory::createExporter(format);
        if (exporter) {
            bool success = exporter->exportToFile(tokens, exportFile);
            if (success) {
                std::cout << "Tokens exported to " << exportFile << " in " << exportFormat << " format." << std::endl;
            } else {
                std::cerr << "Failed to export tokens to " << exportFile << std::endl;
                return false;
            }
        }
    }
    return !lexer.hasErrors();
}

void interactiveMode(const std::string& language = "cpp", const std::string& configFile = "") {
    std::string line;

    LanguageConfig config = getLanguageConfig(language, configFile);

    std::cout << "Lex Interactive Mode (Language: " << config.getName() << ")" << std::endl;
    std::cout << "Enter 'exit' to quit, 'language <name>' to change language," << std::endl;
    std::cout << "'export-config <file>' to export current language config to a file," << std::endl;
    std::cout << "'load-config <file>' to load a language config from a file," << std::endl;
    std::cout << "'list-plugins' to show available language plugins" << std::endl;

    auto& pluginManager = LanguagePluginManager::getInstance();

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        if (line == "exit") {
            break;
        }

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

bool exportLanguageConfig(const std::string& language, const std::string& filename) {
    LanguageConfig config = getLanguageConfig(language);

    bool success = ConfigLoader::saveLanguageToFile(config, filename);
    if (success) {
        std::cout << "Language configuration for " << config.getName() << " exported to: " << filename << std::endl;
    } else {
        std::cerr << "Failed to export language configuration to file" << std::endl;
    }
    return success;
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
    auto& pluginManager = LanguagePluginManager::getInstance();

    std::string filename;
    std::string language = "c++";
    bool languageSpecified = false;
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
                languageSpecified = true;
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

    if (pluginsDir.empty()) pluginManager.scanForPlugins();
    else pluginManager.setPluginsDirectory(pluginsDir);
    if (pluginManager.getAvailableLanguages().empty() && configFile.empty()) {
        std::cerr << "Error: No language plugins found." << std::endl;
        return 1;
    }
    if (argc == 1) {
        interactiveMode();
        return 0;
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
        try {
            return exportLanguageConfig(exportConfigLang, exportConfigFile) ? 0 : 1;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }

    if (interactive) {
        try {
            interactiveMode(language, configFile);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }

    if (!filename.empty()) {

        if (!languageSpecified) {
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

        try {
            return processFile(filename, language, verbose, exportFormat, exportFile, configFile) ? 0 : 1;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "No input file specified" << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}
