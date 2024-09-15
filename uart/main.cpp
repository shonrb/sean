/*
 * uart
 * UART hardware modelled in verilog
 */
#include "VMain.h"
#include "verilated.h"
#include <iostream>
#include <bitset>
#include <string>

void log_byte(std::string verb, char byte)
{
    std::cout 
        << verb   << " "  << std::bitset<8>(byte)
        << " (\'" << byte << "\')\n";
}

int main(int argc, char** argv) 
{
    VerilatedContext* context = new VerilatedContext;
    context->commandArgs(argc, argv);
    VMain* top = new VMain{context};
    top->clk = 0;

    std::string res = "";
    std::string phrase = "Hello, World!";
    int i = 0;
    int prev_busy = 1;
    int prev_read_done = 0;

    while (!context->gotFinish()) {
        top->eval();
        top->clk = !top->clk;
        // If the transmitter just became available, give it a byte to send
        if (prev_busy && !top->busy) {
            if (i == phrase.size()) {
                top->transmit = 0;
            } else {
                top->transmit = 1;
                char c = phrase[i++];
                top->in = c;
                log_byte("Sent", c);
            }
        }  
        // If the receiver just finished reading, get the byte that was read
        if (!prev_read_done && top->read_done) {
            res += top->out;
            if (res.size() == phrase.size())
                top->finish = 1;
            log_byte("Received", top->out);
        }
        prev_busy = top->busy;
        prev_read_done = top->read_done;
    }
    std::cout 
        << "Done. Sent \""      << phrase
        << "\" and received \"" << res << "\"\n";

    delete top;
    delete context;
    return 0;
}
