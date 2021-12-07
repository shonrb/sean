/* bf.cpp
 * A brainfuck interpreter.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

int main(int argc, const char** argv)
{
    if (argc == 1) {
        std::cout << "An input file is required\n";
        return 1;
    }
    // Load the given program
    std::ifstream file(argv[1]);
    std::string script( 
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>(    )));

    constexpr std::size_t MEMORY = 30000;

    std::uint8_t cells[MEMORY]   = {0};
    std::size_t  instruction_ptr = 0;
    std::size_t  data_ptr        = 0;

    // Helper function to step IP to a matching symbol
    // example: [   [   [      ]    [     ]     ]    ]
    //              ^---------------------------^ 
    auto step_to_matching = [&](int dx, char from, char to) {
        unsigned level = 0;
        while (true) {
            instruction_ptr += dx;
            char sym = script[instruction_ptr];
            if      (sym == from)             level++;
            else if (sym == to && level != 0) level--;
            else if (sym == to && level == 0) break;
        }
    };

    // Run the program
    for (; instruction_ptr < script.size(); ++instruction_ptr) {
        char instr = script[instruction_ptr];
        uint8_t& cell = cells[data_ptr];

        switch (instr) {
        case '>': data_ptr++;                                break;
        case '<': data_ptr--;                                break;
        case '+': cell++;                                    break;
        case '-': cell--;                                    break;
        case '.': putchar((char) cell);                      break;
        case ',': cell = getchar();                          break;
        case '[': if (!cell) step_to_matching(1,  '[', ']'); break;
        case ']': if (cell)  step_to_matching(-1, ']', '['); break;
        }
    }
}