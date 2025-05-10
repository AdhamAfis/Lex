#include <iostream>
#include <fstream>
#include <sstream>
#include "Lexer.h"

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

void processString(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    std::cout << "Tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::cout << token.toString() << std::endl;
    }
}

void processFile(const std::string& filename) {
    std::string source = readFile(filename);
    if (!source.empty()) {
        processString(source);
    }
}

void interactiveMode() {
    std::string line;
    std::cout << "Lex Interactive Mode (Enter 'exit' to quit)" << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line == "exit") {
            break;
        }
        
        processString(line);
    }
}

void printUsage() {
    std::cout << "Usage: lex [options] [file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --interactive    Start in interactive mode" << std::endl;
    std::cout << "  -h, --help           Display this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        interactiveMode();
        return 0;
    }
    
    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        printUsage();
        return 0;
    }
    
    if (arg == "-i" || arg == "--interactive") {
        interactiveMode();
        return 0;
    }
    
    // Assume it's a file to process
    processFile(arg);
    return 0;
} 