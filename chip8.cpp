/* chip8.cpp
 * A chip-8 emulator/interpreter
 */
#include <cstdint>
#include <array>
#include <functional>
#include <map>
#include <random>

// The chip-8 includes a builtin 4x5 hex font.
#define FONT {\
    0xF0,0x90,0x90,0x90,0xF0,/*0*/\
    0x20,0x60,0x20,0x20,0x70,/*1*/\
    0xF0,0x10,0xF0,0x80,0xF0,/*2*/\
    0xF0,0x10,0xF0,0x10,0xF0,/*3*/\
    0x90,0x90,0xF0,0x10,0x10,/*4*/\
    0xF0,0x80,0xF0,0x10,0xF0,/*5*/\
    0xF0,0x80,0xF0,0x90,0xF0,/*6*/\
    0xF0,0x10,0x20,0x40,0x40,/*7*/\
    0xF0,0x90,0xF0,0x90,0xF0,/*8*/\
    0xF0,0x90,0xF0,0x10,0xF0,/*9*/\
    0xF0,0x90,0xF0,0x90,0x90,/*A*/\
    0xE0,0x90,0xE0,0x90,0xE0,/*B*/\
    0xF0,0x80,0x80,0x80,0xF0,/*C*/\
    0xE0,0x90,0x90,0x90,0xE0,/*D*/\
    0xF0,0x80,0xF0,0x80,0xF0,/*E*/\
    0xF0,0x80,0xF0,0x80,0x80 /*F*/\
}

class Chip8Cpu {
    static constexpr uint8_t  WIDTH  = 64;
    static constexpr uint8_t  HEIGHT = 32;
    // Bytes 0 - 0x200 are reserved for the interpreter
    static constexpr uint16_t PROG_START = 0x200; 
    union {
        std::array<uint8_t, 4096> memory = {0};

        struct {
            std::array<uint8_t,  16> V;
            std::array<uint16_t, 16> stack;

            uint16_t I;
            uint16_t PC;
            uint8_t  SP;
            uint8_t  delay_timer;
            uint8_t  sound_timer;

            std::array<uint8_t, 16>      keyboard;
            std::array<uint8_t, 64*32/8> display;
            std::array<uint8_t, 16*5>    font;

            uint8_t waiting;
            uint8_t key_reg;
        } res;
    };
    std::mt19937 rnd{};
    uint32_t bg_colour = 0x000000;
    uint32_t fg_colour = 0xFFFFFF;

public:
    Chip8Cpu() {
        res.PC = PROG_START;
        // Load the font
        int offset = 0;
        for (unsigned byte : FONT) {
            res.font[offset++] = byte;
        }
    }

    void load_ROM(auto& file, auto&& not_empty, auto&& readbyte) {
        for (int i = PROG_START; not_empty(file); ++i) {
            memory[i] = readbyte(file);
        }
    }

    void process_key(uint8_t key, uint8_t val) {
        res.keyboard[key] = val;

        // If we are currently waiting on a key, stop 
        // and copy the keycode to V[key_reg].
        if (val == 1 && res.waiting) {
            res.waiting = 0;
            res.V[res.key_reg] = key;
        }
    }

    bool is_waiting() const { 
        return res.waiting == 1;
    }

    void update_timers() {
        // If the timers are above 0, decrement them
        res.delay_timer -= std::min(1, (int) res.delay_timer);
        res.sound_timer -= std::min(1, (int) res.sound_timer);
    }

    std::array<uint32_t, WIDTH*HEIGHT> get_display_buffer() const {
        std::array<uint32_t, WIDTH*HEIGHT> buf;

        for (int x = 0; x < WIDTH; ++x) 
        for (int y = 0; y < HEIGHT; ++y) {
            int index = x + y * WIDTH;
            // Isolate the bit at the given index
            uint8_t val = (res.display[index / 8] >> (7 - index % 8)) & 1;

            uint32_t pixel;
            if (val) pixel = fg_colour;
            else     pixel = bg_colour;

            buf[index] = 0xFF000000 | pixel;
        }
        
        return buf;
    }

    void next_instruction() {
        uint8_t opcode_byte_0 = memory[ res.PC      & 0xFFF];
        uint8_t opcode_byte_1 = memory[(res.PC + 1) & 0xFFF];
        uint16_t opcode       = (opcode_byte_0 << 8) + opcode_byte_1;
        res.PC += 2;
        do_instruction(opcode);
    }

private:
    void draw_sprite(uint8_t regx, uint8_t regy, uint8_t num_bytes) {
        uint8_t x   = res.V[regx];
        uint8_t y   = res.V[regy];
        bool erased = false;

        // Helper function to write a byte to the display memory
        auto put = [&](uint8_t byteoffset, uint8_t doff, uint8_t new_byte) {
            uint8_t ix = (x + doff)       % WIDTH;
            uint8_t iy = (y + byteoffset) % HEIGHT;
            uint8_t index = (ix + iy * WIDTH) / 8;
            uint8_t prev_byte = res.display[index];

            res.display[index] ^= new_byte;

            // Check if any pixels were erased
            if (~(~prev_byte | res.display[index])) 
                erased = true;
        };

        for (uint8_t i = 0; i < num_bytes; i++) {
            uint8_t sprite_byte = memory[(res.I + i) & 0xFFF];
            // Sprites can cross between two bytes
            put(i, 0, sprite_byte >>     (x % 8));
            put(i, 7, sprite_byte << (8 - x % 8));
        }
        res.V[0xF] = erased;
    }

    void do_instruction(uint16_t opcode) {
        // Variables used by the instructions
        uint16_t nnn = opcode & 0xFFF;
        uint8_t  n   = opcode & 0xF;
        uint8_t  x   = (opcode >> 8) & 0xF;
        uint8_t  y   = (opcode >> 4) & 0xF;
        uint8_t  kk  = opcode & 0xFF;

        // Main identifying part of the opcode
        uint8_t high_nybble = opcode >> 12;

        // Use tables to retrieve instructions based on their opcode
        using OpTable = std::map<uint8_t, std::function<void()>>;
        
        // Ops with high nybble 0
        static OpTable sub_ops_0 = {
            { 0xEE,  [&]() { res.PC = res.stack[res.SP-- % 12]; } }, // RET
            { 0xE0,  [&]() { std::fill(res.display.begin(),          // CLS
                                       res.display.end(), 0); } },
        };

        // Ops with high nybble 8: various arithmetic operations
        static OpTable sub_ops_8 = {
            { 0x0,  [&](){ res.V[x] =  res.V[y]; }}, // LD Vx, byte 
            { 0x1,  [&](){ res.V[x] |= res.V[y]; }}, // OR Vx, Vy
            { 0x2,  [&](){ res.V[x] &= res.V[y]; }}, // AND Vx, Vy
            { 0x3,  [&](){ res.V[x] ^= res.V[y]; }}, // XOR Vx, Vy
            { 0x4,  [&](){ // ADD Vx, Vy
                uint16_t val = res.V[x] + res.V[y];
                res.V[0xF] = val > 255;
                res.V[x]   = val & 0xFF;
            }},
            { 0x5,  [&](){ // SUB Vx, Vy
                res.V[0xF] =  res.V[x] > res.V[y];
                res.V[x]   -= res.V[y];
            }},
            { 0x6,  [&](){ // SHR Vx {, Vy}
                res.V[0xF] =   res.V[x] & 1;
                res.V[x]   >>= 1;
            }},
            { 0x7,  [&](){ // SUBN Vx, Vy
                res.V[0xF] = res.V[x] < res.V[y];
                res.V[x]   = res.V[y] - res.V[x];
            }},                                       
            { 0xE,  [&](){ // SNE Vx, Vy
                res.V[0xF] =   res.V[x] >> 7;
                res.V[x]   <<= 1;
            }}
        };

        // Ops with high nybble E: Keyboard conditional skips
        static OpTable sub_ops_E = {
            { 0x9E,  [&](){ if (res.keyboard[res.V[x]])  res.PC += 2; }}, // SKP Vx
            { 0xA1,  [&](){ if (!res.keyboard[res.V[x]]) res.PC += 2; }}  // SKNP Vx
        };

        // Ops with high nybble F: Various loading & storing operations
        static OpTable sub_ops_F = {
            { 0x07,  [&](){ res.V[x] = res.delay_timer; }}, // LD Vx, DT
            { 0x0A,  [&](){                                 // LD Vx, K
                res.waiting = 1; 
                res.key_reg = x;
            }},
            { 0x15,  [&](){ res.delay_timer = res.V[x]; }}, // LD DT, Vx
            { 0x18,  [&](){ res.sound_timer = res.V[x]; }}, // LD ST, Vx
            { 0x1E,  [&](){ res.I += res.V[x]; }},          // ADD I, Vx
            { 0x29,  [&](){                                 // LD F, Vx
                res.I = offsetof(Chip8Cpu, res.font) + (res.V[x] & 0xF) * 5;
            }},
            { 0x33,  [&](){ // LD B, Vx
                memory[res.I       & 0xFFF] = (res.V[x] / 100) % 10;
                memory[(res.I + 1) & 0xFFF] = (res.V[x] / 10)  % 10;
                memory[(res.I + 2) & 0xFFF] =  res.V[x]        % 10;
            }},
            { 0x55,  [&](){ // LD [I], Vx
                for (uint8_t i = 0; i <= x; ++i) 
                    memory[(res.I + i) & 0xFFF] = res.V[i]; 
            }},
            { 0x65,  [&](){ // LD Vx, [I]
                for (uint8_t i = 0; i <= x; ++i)             
                    res.V[i] = memory[(res.I + i) & 0xFFF];
            }}
        };

        static OpTable main_ops = {
            /* Jumping */
            { 0x1,  [&](){ res.PC = nnn; }},            // JP addr
            { 0xB,  [&](){ res.PC = nnn + res.V[0]; }}, // JP V0, addr
            { 0x2,  [&](){                              // CALL addr
                res.stack[++res.SP % 12] = res.PC; 
                res.PC = nnn;
            }},
            /* Skipping */
            { 0x3,  [&](){ if (res.V[x] == kk)       res.PC += 2; }}, // SE Vx, byte
            { 0x4,  [&](){ if (res.V[x] != kk)       res.PC += 2; }}, // SNE Vx, byte
            { 0x5,  [&](){ if (res.V[x] == res.V[y]) res.PC += 2; }}, // SE Vx, Vy
            { 0x9,  [&](){ if (res.V[x] != res.V[y]) res.PC += 2; }}, // SNE Vx, Vy
            /* Immediate loading */
            { 0x6,  [&](){ res.V[x] =  kk; }},  // LD Vx, byte
            { 0x7,  [&](){ res.V[x] += kk; }},  // ADD Vx, byte
            { 0xA,  [&](){ res.I    =  nnn; }}, // LD I, addr
            /* Other */
            { 0xD,  [&](){ draw_sprite(x, y, n); }}, // DRW Vx, Vy, nibble
            { 0xC,  [&](){                           // RND Vx, byte
                uint8_t num = std::uniform_int_distribution<>(0, 255)(rnd);
                res.V[x]    = num & kk;
            }}, 
            /* Operations which share a highest nybble */
            { 0x0,  [&](){ sub_ops_0[kk](); }},
            { 0x8,  [&](){ sub_ops_8[n](); }},                        
            { 0xE,  [&](){ sub_ops_E[kk](); }},              
            { 0xF,  [&](){ sub_ops_F[kk](); }},
        };

        main_ops[high_nybble]();
    }
};



#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <fstream>

static void handle_events(Chip8Cpu& cpu, bool& running) {
    std::map<int, uint8_t> key_to_offset = {
        {SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
        {SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
        {SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
        {SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF}
    };
    
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            int sym    = event.key.keysym.sym;
            bool press = event.type == SDL_KEYDOWN;

            if (key_to_offset.find(sym) != key_to_offset.end()) {
                cpu.process_key(key_to_offset[sym], press);
            }

            if (sym != SDLK_ESCAPE) {
                break;
            }
            [[fallthrough]];
        }
        case SDL_QUIT:
            running = false;
            break;
        }
    }
}

int main(int argc, const char** argv) {
    constexpr int OPS_PER_FRAME = 100;
    constexpr int FPS           = 60;
    constexpr int SCREEN_WIDTH  = 640;
    constexpr int SCREEN_HEIGHT = 320;

    if (argc != 2) {
        std::cout << "A ROM is required\n";
        return 1;
    }

    // Set up CPU
    Chip8Cpu cpu = Chip8Cpu();
    std::ifstream file(argv[1], std::ios::binary);
    cpu.load_ROM(
        file, 
        [](auto& file){ return file.good(); }, 
        [](auto& file){ return file.get(); });

    // Set up graphics
    SDL_Window* window = SDL_CreateWindow(
        "Chip-8", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 
        SDL_WINDOW_RESIZABLE);

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, 0);

    SDL_Texture*  texture  = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 64, 32);
    
    bool draw_frame    = false;
    int  frames_so_far = 0;
    auto start         = std::chrono::system_clock::now();

    for (bool running = true; running;) {
        handle_events(cpu, running);

        // Only draw a new frame every 1 / FPS seconds
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - start;
        draw_frame = int(elapsed_seconds.count() * FPS) - frames_so_far > 0;

        if (draw_frame) {
            ++frames_so_far;
            cpu.update_timers();

            // Copy the display buffer to the screen
            auto buf = cpu.get_display_buffer();
            SDL_UpdateTexture(texture, nullptr, &buf, 4*64);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        } else {
            SDL_Delay(1000 / FPS);
        }

        if (!cpu.is_waiting()) {
            for (int _ = 0; _ < OPS_PER_FRAME; ++_) {
                cpu.next_instruction();
            }
        }
    }
    SDL_Quit();
}
