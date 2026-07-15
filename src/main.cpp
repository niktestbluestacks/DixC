// dix
#include <Parser.hpp>
#include <SemanticAnalyzer.hpp>

// std
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

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
        auto diagnostics = analyzer.analyze(ast.get());
        
        if (!diagnostics.empty()) {
            bool has_errors = false;
            for (const auto& diag : diagnostics) {
                std::cerr << argv[1] << " :" << diag.line << ":" << diag.column << 
                    (diag.kind == dix::DiagnosticKind::Warning ? " warning " : " error ") <<
                    diag.message << std::endl;
                if (diag.kind == dix::DiagnosticKind::Error) {
                    has_errors = true;
                }
            }
            if (has_errors) return 1;
        }
        
        std::cout << "  No semantic errors!\n";
    } catch (const std::exception& e) {
        std::cerr << "Compalation Error: " << e.what() << "\n";
        return 1;
    }
}