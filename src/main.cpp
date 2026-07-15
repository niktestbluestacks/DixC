// dix
#include <Parser.hpp>

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
        dix::Parser parser(source);
        auto ast = parser.parseProgram();
        std::cout << "  Parsing successful!\n";
        std::cout << "  Top-level declarations: " 
                  << dynamic_cast<dix::BlockStatement*>(ast.get())->statements.size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "  Parse error: " << e.what() << "\n";
        return 1;
    }
}