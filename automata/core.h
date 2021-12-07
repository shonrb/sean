#ifndef __CORE_H_
#define __CORE_H_

#include <stdbool.h>

#define DEAD 0
#define MAX_RULESET 10
#define VEC2I(x, y) (union vec2i) {{x, y}} 

union vec2i {
    struct { int x, y; };
    struct { int width, height; };  
};

struct ruleset {
    int range[MAX_RULESET];
    int threshold[MAX_RULESET];
    int states;
    int alive;

    int range_count;
    int threshold_count;

    bool valid;
    char *error_msg;
};

struct view {
    union vec2i offset;
    float scale;
    bool paused;
};

struct board {
    union vec2i dims;
    int area;

    struct ruleset rules;
    struct view view;
    
    int paint_value;
    bool painting;

    int *cells;
};

struct ruleset parse_rule_string  (const char *string);
union vec2i    world_to_screen    (struct view view, union vec2i world);
union vec2i    screen_to_world    (struct view view, union vec2i screen);
void           pan                (struct view *view, union vec2i delta);
void           zoom               (struct view *view, float multiplier, union vec2i centre);
struct board   *create_board      (struct ruleset rules, int width, int height);
void           randomise_board    (struct board* board);
void           advance_generation (struct board* board);

#endif 