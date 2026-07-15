// dix
#include <Parser.hpp>
#include <SemanticAnalyzer.hpp>

// std
#include <iostream>
#include <fstream>
#include <sstream>

int main(/*int argc, char* argv[]*/) {
    int argc = 2;
    char* argv[] = {nullptr, "../../src/test.c"};
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << "<source.c>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    try {
        dix::Parser parser{source};
        auto ast = parser.parseProgram();
        
        dix::SemanticAnalyzer analyzer;
        auto errors = analyzer.analyze(ast.get());
        
        if (!errors.empty()) {
            for (const auto& err : errors) {
                std::cerr << argv[1] << ":" << err.line << ":" << err.column 
                          << ": error: " << err.message << "\n";
            }
            return 1;
        }
        
        std::cout << "✓ No semantic errors!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}