#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "core.h"

#define INIT_VIEW       \
(struct view) {         \
    .offset = {{0, 0}}, \
    .scale = 1.0f,      \
    .paused = false     \
}

static inline int char_to_int(char c)
{
    return c - '0';
}

static inline int is_invalid(int i)
{
    return i < 0 || i > 9;
}

static struct ruleset make_random_ruleset()
{
    // TODO: use a better rng
    srand(time(NULL));
    struct ruleset rules;
    rules.valid = true; // Rules generated here are always valid

    rules.range_count     = rand() % MAX_RULESET;
    rules.threshold_count = rand() % MAX_RULESET;
    rules.states          = rand() % 8 + 2; // Between 2 and 9
    rules.alive           = rules.states - 1;

    for (int i = 0; i < rules.range_count; ++i)
        rules.range[i] = rand() % 10;

    for (int i = 0; i < rules.threshold_count; ++i)
        rules.threshold[i] = rand() % 10;

    return rules;
}

static void validate_ruleset_values(struct ruleset *rules)
{
    /* Check that all of the values in the ruleset 
       are in range */
    for (int i = 0; i < rules->range_count; ++i) {
        if (is_invalid(rules->range[i])) {
            rules->error_msg = "invalid range value";
            rules->valid = false;
        }
    }

    for (int i = 0; i < rules->threshold_count; ++i) {
        if (is_invalid(rules->threshold[i])) {
            rules->error_msg = "invalid threshold value";
            rules->valid = false;
        }
    }

    if (rules->states < 2 || is_invalid(rules->states)) {
        rules->error_msg = "invalid states value";
        rules->valid = false;
    }
}

struct ruleset parse_rule_string(const char *string)
{
    /* Parses a string with format "ranges/threshold/state"
       into a ruleset structure for use by the simulation */
    if (*string == 'R') 
        return make_random_ruleset();

    // If non-random ruleset, parse the given string into a ruleset.
    struct ruleset rules;

    // Create a mutable copy of the string to split
    char *copy = malloc(strlen(string));
    strcpy(copy, string);
    char *delimiter = "/";

    // Split the string into 3
    char *range_str     = strsep(&copy, delimiter);
    char *threshold_str = strsep(&copy, delimiter);
    char *states_str    = strsep(&copy, delimiter);
    free(copy);

    // Initialise the other fields
    rules.range_count     = strlen(range_str);
    rules.threshold_count = strlen(threshold_str);
    rules.states          = char_to_int(*states_str);
    rules.alive           = rules.states - 1;
    rules.valid           = true;

    if (rules.range_count <= MAX_RULESET && rules.threshold_count <= MAX_RULESET) {
        // Extract the values from the range and threshold strings.
        for (unsigned i = 0; i < strlen(range_str); ++i) 
            rules.range[i] = char_to_int(range_str[i]);

        for (unsigned i = 0; i < strlen(threshold_str); ++i) 
            rules.threshold[i] = char_to_int(threshold_str[i]);

        // Ensure valid values were given. If not, .valid will be set to false.        
        validate_ruleset_values(&rules);
    } else {
        rules.valid = false;
        rules.error_msg = "Too many values given";
    }
    return rules;
}

union vec2i world_to_screen(struct view view, union vec2i world)
{
    /* Converts world coordinates to screen coordinates. */
    return (union vec2i) {{
        (world.x - view.offset.x) * view.scale,
        (world.y - view.offset.y) * view.scale
    }};
}

union vec2i screen_to_world(struct view view, union vec2i screen)
{
    /* Converts screen coordinates to world coordinates. */
    return (union vec2i) {{
        (screen.x / view.scale) + view.offset.x,
        (screen.y / view.scale) + view.offset.y
    }};
}

void pan(struct view *view, union vec2i delta)
{
    view->offset.x += delta.x;
    view->offset.y += delta.y;
}

void zoom(struct view *view, float multiplier, union vec2i towards)
{
    // Convert the zooming point to world space.
    union vec2i before = screen_to_world(*view, towards);

    // Scale the view, then convert the zooming point again.
    view->scale *= multiplier;
    union vec2i after = screen_to_world(*view, towards);

    // Pan the view by the difference between them to centre the view again.
    union vec2i offset = {{
        -(after.x - before.x),
        -(after.y - before.y)
    }};
    
    pan(view, offset);
}

struct board* create_board(struct ruleset rules, int width, int height)
{
    struct board* board = malloc(sizeof(struct board));

    size_t size = sizeof(int) * width * height;
    board->cells = malloc(size);
    memset(board->cells, 0, size);

    board->rules       = rules;
    board->dims        = VEC2I(width, height);
    board->area        = width * height;
    board->view        = INIT_VIEW;
    board->paint_value = 0;
    board->painting    = false;

    return board;
}

void randomise_board(struct board* board)
{
    // TODO: better prng
    srand(time(NULL));

    for (int i = 0; i < board->area; ++i) {
        if (rand() % 6) 
            board->cells[i] = DEAD;
        else 
            board->cells[i] = board->rules.alive;
    }
}

static int get_neighbour_count(struct board* board, int x, int y)
{
    // Active cells are the highest valid state for the ruleset.
    int alive  = board->rules.alive;
    int width  = board->dims.width;
    int lastx  = board->dims.width - 1;
    int lasty  = board->dims.height - 1;
    int *cells = board->cells;

    // Search the moore neighbouring cells of [x, y] for live cells.
    // Fix this later btw its revolting.
    int count 
    = (x < lastx               && cells[(x+1) +  y    * width] == alive)
    + (x > 0                   && cells[(x-1) +  y    * width] == alive)
    + (y < lasty               && cells[ x    + (y+1) * width] == alive)
    + (y > 0                   && cells[ x    + (y-1) * width] == alive)
    + (x < lastx && y < lasty  && cells[(x+1) + (y+1) * width] == alive)
    + (x < lastx && y > 0      && cells[(x+1) + (y-1) * width] == alive)
    + (x > 0     && y < lasty  && cells[(x-1) + (y+1) * width] == alive)
    + (x > 0     && y > 0      && cells[(x-1) + (y-1) * width] == alive);

    return count;
}

static int apply_rules_to_cell_value(struct ruleset rules, int val, int neighbours)
{
    /* Applies the ruleset to a single cell, 
       using its state and neighbour count. */

    // Dead cells.
    if (val == DEAD) {
        for (int i = 0; i < rules.threshold_count; ++i) {
            if (neighbours == rules.threshold[i]) {
                return rules.alive;
            }
        }
        return DEAD;
    }

    // Active cells.
    if (val == rules.alive) {
        for (int i = 0; i < rules.range_count; ++i) {
            if (neighbours == rules.range[i]) {
                return val;
            }        
        }
    }

    // Refractory cells.
    return val - 1;
}

void advance_generation(struct board* board)
{
    // Create a buffer for the next generation.
    int *new = malloc(sizeof(int) * board->area);

    // Walk through the old array and apply rules to each cell.
    for (int x = 0; x < board->dims.width; ++x) {
        for (int y = 0; y < board->dims.height; ++y) {
            int neighbours = get_neighbour_count(board, x, y);
            int index = x + y * board->dims.width;
            int val = board->cells[index];

            // Add the new value to the buffer.
            new[index] = apply_rules_to_cell_value(board->rules, val, neighbours);
        }
    }

    // Free old generation and replace with the new one.
    free(board->cells);
    board->cells = new;
}