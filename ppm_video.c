/* Writes an animation to stdout in PPM format.
   Usage:  ./prog | mpv --no-correct-pts --geometry=600x600 --fps=30 -
*/
#include <math.h>
#include <stdint.h>

// We need fmin and fmax equivalents which work on any type
#define MAX(v0, v1) (v0 > v1 ? v0 : v1)
#define MIN(v0, v1) (v0 < v1 ? v0 : v1)
#define CLAMP(v, min, max) (MIN(max, MAX(min, v)))

struct rgb {
    uint8_t r, g, b;
};

static float lerp(float v0, float v1, float weight) 
{
    return v0 * (1 - weight) + v1 * weight;
}

static void 
draw_circle(struct rgb *buffer, 
            int width,        int height, 
            float cx,         float cy, 
            float out_radius, float in_radius,   
            struct rgb colour) 
{
    // Calulate the bounds of the circle
    int x_min = CLAMP(floorf(cx - out_radius - 1), 0, width);
    int x_max = CLAMP(ceilf( cx + out_radius + 1), 0, width);
    int y_min = CLAMP(floorf(cy - out_radius - 1), 0, height);
    int y_max = CLAMP(ceilf( cy + out_radius + 1), 0, height);

    for (int x = x_min; x < x_max; ++x) {
        float dx = x - cx;

        for (int y = y_min; y < y_max; ++y) {
            // Only draw a pixel if it's within the outer radius
            float dy    = y - cy;
            float dist2 = dx*dx + dy*dy;
            float out2  = out_radius*out_radius;

            if (dist2 < out2) {
                // Smoothstep the disance between the radii
                float in2 = in_radius*in_radius;
                float sat = CLAMP((dist2 - out2) / (in2 - out2), 0.0f, 1.0f);
                float w   = sat * sat * (3 - 2 * sat);
                // Composite the colour onto the buffer, with a
                // lower alpha the further from the inner radius
                int    index          = x + y * width;
                struct rgb old_colour = buffer[index];
                struct rgb new_colour = {
                    (uint8_t) lerp(old_colour.r, colour.r, w),
                    (uint8_t) lerp(old_colour.g, colour.g, w),
                    (uint8_t) lerp(old_colour.b, colour.b, w),
                };
                buffer[index] = new_colour;
            }
        }
    }
}

#include <stdio.h>

#define WIDTH  255
#define HEIGHT 255
#define MAXVAL 255
#define FRAMES 500
#define RADIUS_INNER 2
#define RADIUS_OUTER 5
#define NUM_CIRCLES 250
#define X_OFF -(WIDTH / 2.5)
#define Y_OFF -(HEIGHT / 2.5)
#define MULT 300.0
#define SPEED 0.005

int main(void) 
{
    for (int frame = 0; frame < FRAMES; ++frame) {
        struct rgb screen_buffer[WIDTH*HEIGHT] = {0};
        
        // Draw circles based on an equation
        for (int i = 0; i < NUM_CIRCLES; i++){
            float offset = i * ((M_PI*2) / NUM_CIRCLES);
            float x = cos(sin(offset * frame*SPEED)) * MULT + X_OFF;
            float y = cos(sin(offset - frame*SPEED)) * MULT + Y_OFF; 

            struct rgb colour = {
                (sin(i / 100.0) + 1) / 2 * 255,
                (cos(i / 100.0) + 1) / 2 * 255,
                255
            };

            draw_circle(
                screen_buffer, WIDTH, HEIGHT, x, y, 
                RADIUS_OUTER, RADIUS_INNER, colour);
        }

        // Write the buffer to stdout in ppm format
        fprintf(stdout, "P3\n%d %d\n%d\n", WIDTH, HEIGHT, MAXVAL);
        for (int x = 0; x < WIDTH; ++x) 
        for (int y = 0; y < HEIGHT; ++y) {
            struct rgb col = screen_buffer[x + y * WIDTH];
            fprintf(stdout, " %d %d %d", col.r, col.g, col.b);
        }
    }
}
