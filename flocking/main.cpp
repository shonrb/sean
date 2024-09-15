/* flocking
 * A flocking simulation based on https://www.red3d.com/cwr/boids/
 */

#include <SDL2/SDL.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "flock.hpp"

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

union Argb {
    struct {
        unsigned char b, g, r, a;
    } channels;
    unsigned colour;
};

static void plot_line(std::array<unsigned, WIDTH*HEIGHT>& pixel_buffer,     
                      unsigned colour,
                      int x0, int y0,
                      int x1, int y1)
{
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    float dx    = x1 - x0;
    float dy    = std::abs(y1 - y0);
    float error = dx / 2.0f;

    int ystep = (y0 < y1) ? 1 : -1;
    int y     = y0;

    auto put_pixel = [&](int x, int y) {
        pixel_buffer[x + y * WIDTH] = colour;
    };
    
    for (int x = x0; x < x1; ++x) {
        if (steep) put_pixel(y, x);
        else       put_pixel(x, y);
    
        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

int main()
{
    constexpr int NUM_BOIDS = 300;
    constexpr float TRAIL_DISSIPATION = 0.9;

    // Create graphics objects
    SDL_Window *window = SDL_CreateWindow(
        "Flocking", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, 0);
    
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    std::array<unsigned, WIDTH*HEIGHT> pixel_buffer = {0};

    // Initialise the flock
    Flock flock(WIDTH, HEIGHT, NUM_BOIDS);

    // Other UI stuff
    int mouse_x;
    int mouse_y;
    bool mouse_pressed = false;
    bool running = true;

    while (running) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                mouse_pressed = !mouse_pressed;
                break;
            default:
                break;
            }
        }
        SDL_GetMouseState(&mouse_x, &mouse_y);

        // Fade the previous positions of the boids towards black
        std::transform(
            pixel_buffer.begin(), pixel_buffer.end(), pixel_buffer.begin(),
            [](unsigned colour) -> unsigned {
                // Unpack the pixel into it's components, fade them towards 0,
                // then repack them.
                Argb c;
                c.colour = colour;
                c.channels.r *= TRAIL_DISSIPATION;
                c.channels.g *= TRAIL_DISSIPATION;
                c.channels.b *= TRAIL_DISSIPATION;
                return c.colour;
            });

        // Draw each of the boids as a line representing its movement
        flock.draw_boids(
            [&](float x0, float y0, float x1, float y1, unsigned col) {
                x0 = std::clamp(int(x0), 0, WIDTH-1);
                y0 = std::clamp(int(y0), 0, HEIGHT-1);
                x1 = std::clamp(int(x1), 0, WIDTH-1);
                y1 = std::clamp(int(y1), 0, HEIGHT-1);
                plot_line(pixel_buffer, col, x0, y0, x1, y1);
            });
        
        // If the mouse is pressed, steer the flock towards it
        if (mouse_pressed) {
            Vec2f attractor{float(mouse_x), float(mouse_y)};
            flock.update(attractor);
        } else {
            flock.update();
        }

        // Update the display
        SDL_UpdateTexture(texture, nullptr, &pixel_buffer, WIDTH*4);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }
}