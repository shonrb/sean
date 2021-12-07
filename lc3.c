/* lc3.c
 * An LC-3 emulator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/termios.h>

/* UI */

struct termios original_term;

static void disable_input_buffering() 
{
    tcgetattr(STDIN_FILENO, &original_term);
    struct termios new_term = original_term;
    new_term.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
}

static void restore_input_buffering() 
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term);
}

static bool check_for_keypress() 
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout = {
        .tv_sec  = 0,
        .tv_usec = 0
    };
    return select(1, &readfds, NULL, NULL, &timeout);
}

/* HELPER FUNCTIONS */

static uint16_t sign_ext(uint16_t x, size_t len) 
{
    int mask = 1U << (len - 1);
    x &= ((1U << len) - 1); 
    return (x ^ mask) - mask;
}

static uint16_t swap_endianness(uint16_t x) 
{
    return (x << 8) | (x >> 8);
}

/* EMULATOR */

#define LC3_MEM_SIZE   0x10000
#define LC3_PROG_START 0x3000
#define LC3_PROG_END   0xFE00
#define POSITIVE_FLAG  1
#define ZERO_FLAG      2 
#define NEGATIVE_FLAG  4

enum instruction {
    BR = 0, ADD,     LD,     ST,    
    JSR,    AND,     LDR,    STR,   
    RTI,    NOT,     LDI,    STI,   
    JMP,    OP1101,  LEA,    TRAP   
}; 

enum trap_routine {
    GETC  = 0x20,    OUT   = 0x21,
    PUTS  = 0x22,    IN    = 0x23,
    PUTSP = 0x24,    HALT  = 0x25 
};

enum device_register {
    KBSR = 0xFE00, // keyboard status
    KBDR = 0xFE02, // keyboard data
    DSR  = 0xFE04, // display status
    DDR  = 0xFE06, // display data
    MCR  = 0xFFFE  // machine control
};

struct lc3_cpu {
    uint16_t memory[LC3_MEM_SIZE];
    uint16_t reg[8];
    uint16_t pc;
    uint16_t cond;
    bool     running;
};

static void lc3_init(struct lc3_cpu *cpu, FILE* image) 
{
    cpu->pc          = LC3_PROG_START;
    cpu->cond        = 0;
    cpu->running     = true;
    cpu->memory[MCR] = 1 << 15; // Set the clock enable bit
    
    // Load a program image into memory
    // The first two bytes of the image file specify
    // the location in memory to write to
    uint16_t origin;
    fread(&origin, sizeof(uint16_t), 1, image);
    origin = swap_endianness(origin);

    // Read the rest of the image file to memory, starting from origin
    uint16_t  words_to_read = LC3_MEM_SIZE - origin - LC3_PROG_END;
    uint16_t *location      = cpu->memory  + origin;
    size_t    image_size    = fread(
        location, sizeof(uint16_t), words_to_read, image);

    for (unsigned i = 0; i < image_size; ++i) {
        *location = swap_endianness(*location);
        ++location;
    }
}

static void lc3_end_program(struct lc3_cpu *cpu, const char *why) 
{
    printf("Killed Program: %s\n", why);
    cpu->running = false;
}

static void lc3_store_reg(struct lc3_cpu *cpu, uint16_t reg_no, uint16_t val) 
{
    cpu->reg[reg_no] = val;
    if      (val == 0)  cpu->cond = ZERO_FLAG;
    else if (val >> 15) cpu->cond = NEGATIVE_FLAG;
    else                cpu->cond = POSITIVE_FLAG; 
}

static void 
lc3_mem_write(struct lc3_cpu *cpu, uint16_t address, uint16_t val) 
{
    cpu->memory[address] = val;
    uint16_t high_bit = val >> 15;
    
    if (address == DSR && high_bit) {
        // If bit 15 in DSR is set, output the contents of DDR,
        putchar((char) cpu->memory[DDR]);
    } else if (address == MCR && !high_bit) {
        // If the clock bit of MCR is cleared, end execution
        lc3_end_program(cpu, "MCR clock bit cleared");
    }
}

static uint16_t lc3_mem_read(struct lc3_cpu *cpu, uint16_t address) 
{
    // Bit 15 in KBSR is set when a key is pressed
    // If a key is pressed, read a key into KBDR
    if (address == KBSR) {
        if (check_for_keypress()) {
            cpu->memory[KBSR] = (1 << 15);
            cpu->memory[KBDR] = getchar();
        } else {
            cpu->memory[KBSR] = 0;
        }
    }
    return cpu->memory[address];
}

static void lc3_do_trapcode(struct lc3_cpu *cpu, uint16_t trap_vector) 
{
    switch (trap_vector) {
    /* Input */
    case GETC:  // Read character from keyboard into R0
        cpu->reg[0x0] = (uint16_t)getchar();
        break;
    case IN: {  // Read character from keyboard into R0, and echo it to the display
        printf("Input a character: ");
        char c = getchar();
        cpu->reg[0x0] = (uint16_t)c;
        putchar(getchar());
        break;
    }

    /* Output */
    case OUT:    // Write character in R0 to display
        putchar((char)cpu->reg[0x0]);
        break;
    case PUTS: { // Write null-terminated string from mem[R0] to display
        uint16_t* c = cpu->memory + cpu->reg[0x0];
        for (; *c; ++c) {
            putchar((char)*c);
        }
        break;
    }
    case PUTSP: { 
        // Write null-terminated string from mem[R0] to display, 
        // with the high and low bytes as separate characters
        uint16_t* c = cpu->memory + cpu->reg[0x0];
        for (; *c; c++) {
            char char1 = (*c) & 0xFF;
            char char2 = (*c) >> 8;
            putchar(char1);
            if (char2) {
                putchar(char2);
            }
        }
        break;
    }
    case HALT: lc3_end_program(cpu, "HALT Called");  break;
    default:   lc3_end_program(cpu, "Bad Trapcode"); break;
    }
}

static void lc3_next_cycle(struct lc3_cpu *cpu) 
{
    uint16_t opcode = lc3_mem_read(cpu, cpu->pc++);
    uint16_t id = opcode >> 12;

    // Variables used by the instructions
    // Registers
    uint16_t dr    =          (opcode >> 9) & 0x7;
    uint16_t sr1   = cpu->reg[(opcode >> 6) & 0x7];
    uint16_t sr2   = cpu->reg[ opcode       & 0x7];
    uint16_t sr    = cpu->reg[dr];
    uint16_t baser = sr1;
    // Immediate values
    uint16_t imm5       = sign_ext(opcode & 0x1F,     5);
    uint16_t pcoffset11 = sign_ext(opcode & 0x7FF,    11);
    uint16_t pcoffset9  = sign_ext(opcode & 0x1FF,    9);
    uint16_t offset6    = sign_ext(opcode & 0x3F,     6);
    // Identifying bits
    uint16_t bit5       = (opcode >> 5)  & 1;
    uint16_t bit11      = (opcode >> 11) & 1;
    // Other
    uint16_t nzp        = dr; // Condition mask

    switch (id) {
    /* Arithmetic */
    case ADD:  // Addition: DR = SR1 + imm5 if bit5 is set, else SR1 + SR2
        lc3_store_reg(cpu, dr, sr1 + (bit5 ? imm5 : sr2));
        break;
    case AND:  // Bitwise Not: DR = SR1 & imm5 if bit5 is set, else SR1 & SR2
        lc3_store_reg(cpu, dr, sr1 & (bit5 ? imm5 : sr2));
        break;
    case NOT:  // Bitwise Not:  DR = NOT SR1
        lc3_store_reg(cpu, dr, ~sr1);
        break;

    /* Jumping */
    case BR:   // Conditional Branch: PC += PCoffset9 if nzp & COND
        if (nzp & cpu->cond) cpu->pc += pcoffset9;
        break;
    case JMP:  // Jump or Return: PC = SR1
        cpu->pc = sr1;
        break;
    case JSR:  // Jump to subroutine: R7 = PC; PC = PC+PCoffset9 if bit11, else SR1
        cpu->reg[0x7] = cpu->pc;
        cpu->pc = bit11 ? cpu->pc+pcoffset11 : sr1;
        break;

    /* Loading */
    case LD:   // Load: DR = mem[PC + PCoffset9]
        lc3_store_reg(cpu, dr, lc3_mem_read(cpu, cpu->pc + pcoffset9));
        break;
    case LDI:  // Load Indirect: DR = mem[mem[PC + PCoffset9]]
        lc3_store_reg(cpu, dr, 
            lc3_mem_read(cpu, 
                lc3_mem_read(cpu, cpu->pc + pcoffset9)));
        break;
    case LDR:  // Load Base+Offset: DR = mem[BaseR + offset6]
        lc3_store_reg(cpu, dr, lc3_mem_read(cpu, baser + offset6));
        break;
    case LEA:  // Load Effective Address: DR = PC + PCoffset9
        lc3_store_reg(cpu, dr, cpu->pc + pcoffset9);
        break;

    /* Storing */
    case ST:   // Store: mem[PC + PCoffset9] = SR
        lc3_mem_write(cpu, cpu->pc + pcoffset9, sr);
        break;
    case STI:  // Store Indirect: mem[mem[PC + PCoffset9]] = SR
        lc3_mem_write(cpu, lc3_mem_read(cpu, cpu->pc + pcoffset9), sr);
        break;
    case STR:  // Store Base+Offset: mem[BaseR + offset6] = SR
        lc3_mem_write(cpu, sr1 + offset6, sr);
        break;

    /* Other */
    case TRAP:   // System Call
        lc3_do_trapcode(cpu, opcode & 0xFF);
        break;
    case RTI:    // Return from interrupt: Unused
    case OP1101: // Unused
    default:
        lc3_end_program(cpu, "Bad Opcode");
        break;
    }
}

int main(int argc, const char **argv) 
{
    if (argc < 2) {
        puts("A program image file is needed");
        return 1;
    }

    // Configure the terminal
    disable_input_buffering();
    atexit(restore_input_buffering);

    // Init the cpu and load a given image file
    struct lc3_cpu cpu;
    {
        FILE* file = fopen(argv[1], "rb");
        if (!file) { 
            puts("Invalid image");
            return 1;
        };
        lc3_init(&cpu, file);
        fclose(file);
    }

    while (cpu.running) {
        lc3_next_cycle(&cpu);   
        fflush(stdout); // Update the screen if anything was written out
    }
    return 0;
}
