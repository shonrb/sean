#include "VNCO.h"
#include "verilated.h"
#include <cstdio>

auto convert(int sig_width, auto signal) -> double
{

}

auto main(int arg_count, const char **args) -> int 
{
    auto *ctx = new VerilatedContext;
    ctx->commandArgs(arg_count, args);
    auto *top = new VNCO{ctx};

    top->tuning_word = 65535;
    
    while (true) {
        top->clock = !top->clock;
        top->eval();
        if (top->clock)
            printf("%hd\n", top->signal);
    }
}

