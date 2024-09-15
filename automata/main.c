/* automata
 * A visualiser tool for cellular automata
 */
#include <SDL2/SDL.h>
#include <math.h>
#include "core.h"

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define CELLSIZE 3
#define SLEEP_TIME 10
#define PAN_SPEED 5

static void display_ruleset_info(struct ruleset rules)
{  
    for (int i = 0; i < rules.range_count; ++i)
        printf("%d", rules.range[i]);
    printf("/");

    for (int i = 0; i < rules.threshold_count; ++i)
        printf("%d", rules.threshold[i]);
    printf("/%d\n", rules.states);
}

static void menu()
{
    printf(
        "Usage:\n" 
        "./prog R     : Simulate a random ruleset\n"
        "./prog r/t/s : Simulate a given ruleset \n"
        " r : Range     - Neighbour values for a live cell to remain stable\n"
        " t : Threshold - Neighbour values for a dead cell to reactivate\n"
        " s : States    - Range of cell states. The highest is treated as alive,\n"
        "                 while the rest are treated as refactory\n"
        " (r and t can be omitted, or have multiple values)\n"
        "Controls:\n"
        " Space : Pause\n"
        " Mouse : Add / remove cells (left / right)\n"
        " wasd  : Panning\n"
        " zx    : Zoom in/out\n"
        " r     : Randomise\n"
        " c     : Clear\n"
        " f     : Move forward one generation\n"
        "Some Example Rulesets:\n"
        " 23/3/2         : Conway's game of life\n"
        " /2/3           : Brian's brain\n"
    );
}



static void handle_key_input(SDL_KeyboardEvent key, struct board *board)
{   
    union vec2i centre = {{
        SCREEN_WIDTH / 2,
        SCREEN_HEIGHT / 2
    }};

    // Macros to make the key handling code a little clearer.
#define PAN(x, y) pan(&board->view, (union vec2i) {{x, y}})
#define ZOOM(m) zoom(&board->view, m, centre)

    switch (key.keysym.sym) {
    // Movement
    case 'w': PAN(0, -PAN_SPEED); break;
    case 's': PAN(0,  PAN_SPEED); break;
    case 'a': PAN(-PAN_SPEED, 0); break;
    case 'd': PAN( PAN_SPEED, 0); break;
    // Zooming
    case 'x': ZOOM(0.99); break;
    case 'z': ZOOM(1.01); break;
    // Other 
    case 'c': memset(board->cells, 0, sizeof(int) * board->area); break;
    case 'r': randomise_board(board); break;
    case 'f': advance_generation(board); break;
    case ' ': board->view.paused = !board->view.paused; break;
    }

#undef PAN 
#undef ZOOM
}

static void toggle_paint(SDL_MouseButtonEvent mouse, struct board *board)
{
    board->painting = !board->painting;

    // Select the paint type
    if (mouse.button == SDL_BUTTON_LEFT)
        board->paint_value = board->rules.alive;
    else 
        board->paint_value = 0;
}

static void apply_paint(struct board *board)
{
    union vec2i mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);

    union vec2i world = screen_to_world(board->view, mouse);

    // Ensure the mouse position is in bounds
    if (world.x >= 0 && world.x < SCREEN_WIDTH
    &&  world.y >= 0 && world.y < SCREEN_HEIGHT) {
        world.x /= CELLSIZE;
        world.y /= CELLSIZE;

        board->cells[world.x + world.y * board->dims.width] = board->paint_value;
    }
}

static void draw_screen(SDL_Renderer *renderer, struct board *board)
{
    // Clear the screen before proceeding.
    SDL_SetRenderDrawColor(renderer, 50, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Render each cell with a colour based on it's value.
    for (int x = 0; x < board->dims.width;  ++x) {
        for (int y = 0; y < board->dims.height; ++y) {
            int val = board->cells[x + y * board->dims.width];
            int red, blue, green;
            red=blue=green=0;

            // A cell should be drawn black if it's value is dead
            if (val != DEAD) {
                // values have a gradient between blue & white
                int c = 255 * (float)val / (float)board->rules.alive;
                red = green = c;
                blue = 255;
            }

            // Change the world coordinates to screen coordinates and adjust the size
            union vec2i world = {{
                CELLSIZE * x,
                CELLSIZE * y
            }};

            union vec2i screen = world_to_screen(board->view, world);
            int adjusted_size = round(CELLSIZE * board->view.scale) + 1;

            SDL_Rect rect = {screen.x, screen.y, adjusted_size, adjusted_size};
            SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    // Show menu if no arguments are given
    if (argc == 1) {
        menu();
        return 0;
    }

    // Create the ruleset
    struct ruleset rules = parse_rule_string(argv[1]);
    if (!rules.valid) {
        printf("%s\n", rules.error_msg);
        return 0;
    }
    display_ruleset_info(rules);
    

    // Initialise board
    int width  = SCREEN_WIDTH  / CELLSIZE;
    int height = SCREEN_HEIGHT / CELLSIZE;
    struct board*  board = create_board(rules, width, height);
    randomise_board(board);

    // Initialise SDL components
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow(
        "Cellular Automata",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0
    );
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED
    );

    for (;;) {
        draw_screen(renderer, board);

        // Move to the next generation if not paused
        if (!board->view.paused) {
            advance_generation(board);
            SDL_Delay(SLEEP_TIME);
        }

        if (board->painting) {
            apply_paint(board);
        }

        // Handle input
        for (SDL_Event event; SDL_PollEvent(&event);) {
            switch(event.type) {
            case SDL_QUIT:
                goto exit;
            case SDL_KEYDOWN:
                handle_key_input(event.key, board);
                break;
            case SDL_MOUSEBUTTONDOWN: 
            case SDL_MOUSEBUTTONUP:
                toggle_paint(event.button, board);
                break;
            }
        }
    }

exit:
    free(board->cells);
    free(board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
