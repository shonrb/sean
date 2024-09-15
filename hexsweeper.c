#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define TILES_X 20
#define TILES_Y 19
#define MINE_COUNT 80
#define TILE_CIRCUMRADIUS 30.0f
#define TILE_INRADIUS (sqrtf(3.0f) * 0.5f * TILE_CIRCUMRADIUS)
#define TILE_SLOPE_X (sinf(M_PI / 3.0f) * TILE_CIRCUMRADIUS)
#define TILE_SLOPE_Y (cosf(M_PI / 3.0f) * TILE_CIRCUMRADIUS)
#define TILE_X_SPACING (TILE_INRADIUS * 2.0f)
#define TILE_Y_SPACING (TILE_CIRCUMRADIUS * 1.5f)
#define FACE_INRAD_RATIO 0.5
#define WINDOW_X ceil(TILES_X * TILE_X_SPACING)
#define WINDOW_Y ceil(TILES_Y * TILE_Y_SPACING + TILE_SLOPE_Y)
#define TILE_COLOUR_1 (SDL_Colour) {150, 150, 150, 255}
#define TILE_COLOUR_2 (SDL_Colour) {100, 100, 100, 255}

typedef struct Hex 
{
    SDL_FPoint a, b, c, d, e, f;
} Hex;

typedef enum TileType 
{
    TILE_MINE = -1,
    TILE_EMPTY,
    TILE_1,
    TILE_2,
    TILE_3,
    TILE_4,
    TILE_5,
    TILE_6,
} TileType;

typedef enum TileCover
{
    TILE_COVERED = 0,
    TILE_UNCOVERED,
    TILE_FLAGGED,
} TileCover;

typedef struct Tile 
{
    TileType type;
    TileCover cover;
    Hex hex;
    SDL_Rect face;
    int x;
    int y;
} Tile;

Tile nil_tile = {
    .type = TILE_EMPTY,
    .cover = TILE_UNCOVERED,
    .hex = (Hex) { 0 },
    .face = (SDL_Rect) { 0 },
};

static unsigned random_state = 0xfffffff;

static SDL_Texture *digit_1;
static SDL_Texture *digit_2;
static SDL_Texture *digit_3;
static SDL_Texture *digit_4;
static SDL_Texture *digit_5;
static SDL_Texture *digit_6;
static SDL_Texture *mine_tex;
static SDL_Texture *flag_tex;
static SDL_Texture *tick_tex;
static SDL_Texture *cross_tex;

static SDL_Window *window;
static SDL_Renderer *renderer;
static bool running = true;

static bool game_over = false;
static bool first_click = true;
static int flags_placed = 0;
static int flags_correct = 0;

static Tile tiles[TILES_X][TILES_Y];

// Helper

static int row_max(int y)
{
    if (y % 2)
        return TILES_X - 1;
    return TILES_X;
}

static Tile *get_tile(int x, int y)
{
    if (y < 0 || x < 0 || y >= TILES_Y || x >= row_max(y))
        return &nil_tile;
    return &tiles[x][y];
}

static void get_neighbours(Tile *tile, Tile **neighbours)
{
    int x = tile->x;
    int y = tile->y;
    int indent = y % 2 ? x+1 : x-1;
    int xs[] = {x+1, x-1, x,   x,   indent, indent};
    int ys[] = {y,   y,   y+1, y-1, y+1,    y-1};
    int mine_count = 0;
    for (int i = 0; i < 6; ++i) {
        int xx = xs[i];
        int yy = ys[i];
        neighbours[i] = get_tile(xx, yy);
    }
}

static void world_to_screen(int x, int y, float *sx, float *sy)
{
    float fx = (float) x * TILE_X_SPACING + TILE_INRADIUS;
    float fy = (float) y * TILE_Y_SPACING + TILE_CIRCUMRADIUS;
    if (y % 2)
        fx += TILE_INRADIUS;
    *sx = fx;
    *sy = fy;
}

static float to_nearest(float v, int step)
{
    return (( v + (v < 0 ? -step : step) / 2) / step ) * step;    
}

static void screen_to_world(float x, float y, int *wx, int *wy)
{
    y -= TILE_INRADIUS;
    x -= TILE_CIRCUMRADIUS;
    int iy = (int) (to_nearest(y, TILE_Y_SPACING) / TILE_Y_SPACING);
    if (iy % 2) {
        x -= TILE_INRADIUS;
    }
    int ix = (int) (to_nearest(x, TILE_X_SPACING) / TILE_X_SPACING);
    *wx = ix;
    *wy = iy;
}

static int get_random(int begin, int end)
{
	unsigned long product = (unsigned long) random_state * 48271;
	unsigned x = (product & 0x7fffffff) + (product >> 31);
	random_state = (x & 0x7fffffff) + (x >> 31);
    unsigned range = end - begin;
    return (int) (random_state % range) + begin;
}

// Init

static void init_vertices(void) 
{
    for (int y = 0; y < TILES_Y; ++y) {
        for (int x = 0; x < row_max(y); ++x) {
            float fx, fy;
            world_to_screen(x, y, &fx, &fy);
            Hex hex = {
                .a = { fx                , fy + TILE_CIRCUMRADIUS },
                .b = { fx + TILE_SLOPE_X , fy + TILE_SLOPE_Y      },
                .c = { fx + TILE_SLOPE_X , fy - TILE_SLOPE_Y      },
                .d = { fx                , fy - TILE_CIRCUMRADIUS },
                .e = { fx - TILE_SLOPE_X , fy - TILE_SLOPE_Y      },
                .f = { fx - TILE_SLOPE_X , fy + TILE_SLOPE_Y      },
            };
            SDL_Rect face = {
                .x = fx - FACE_INRAD_RATIO * TILE_INRADIUS,
                .y = fy - FACE_INRAD_RATIO * TILE_INRADIUS,
                .w = FACE_INRAD_RATIO * TILE_INRADIUS * 2,
                .h = FACE_INRAD_RATIO * TILE_INRADIUS * 2,
            };
            Tile *tile = get_tile(x, y);
            tile->hex  = hex;
            tile->face = face;
            tile->x    = x;
            tile->y    = y;
        }
    }
}

static void init_mines(void)
{
    for (int _ = 0; _ < MINE_COUNT; ++_) {
redo:
        int my = get_random(0, TILES_Y);
        int mx = get_random(0, row_max(my));
        Tile *tile = &tiles[mx][my];
        if (tile->type == TILE_MINE)
            goto redo;
        tile->type = TILE_MINE;
    }
    for (int y = 0; y < TILES_Y; ++y) {
        for (int x = 0; x < row_max(y); ++x) {
            Tile *tile = get_tile(x, y);
            if (tile->type == TILE_MINE)
                continue;
            int mine_count = 0;
            Tile *neighbours[6];
            get_neighbours(tile, neighbours);
            for (int i = 0; i < 6; ++i) {
                if (neighbours[i]->type == TILE_MINE)
                    mine_count++;
            }
            tile->type = mine_count;
        }
    }
}

static SDL_Texture *make_digit(const unsigned char *dig, SDL_Colour colour) 
{
    int channels = 4;
    unsigned char *pixels = calloc(1, 8 * 8 * channels);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            bool filled = dig[i] & 1 << j;
            if (filled) {
                int off = (j + i * 8) * channels;
                pixels[off + 0] = colour.b;
                pixels[off + 1] = colour.g;
                pixels[off + 2] = colour.r;
                pixels[off + 3] = colour.a;
            }
        }
    }
    SDL_Texture *tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        8,
        8
    );
    SDL_UpdateTexture(tex, NULL, pixels, 8 * channels);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

static void init_textures(void)
{
    static const unsigned char texture_data[][8] = {
        { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},
        { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},
        { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},
        { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},
        { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},
        { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},
        { 0x24, 0x18, 0xad, 0x4e, 0x7e, 0xbd, 0x18, 0x24},
        { 0x18, 0x1e, 0x1f, 0x1e, 0x18, 0x10, 0x10, 0x10},
        { 0x80, 0xc0, 0xc0, 0x60, 0x61, 0x33, 0x3e, 0x1c},
        { 0x81, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3}
    };
    digit_1   = make_digit(texture_data[0], (SDL_Colour){0,   0,   255, 255});
    digit_2   = make_digit(texture_data[1], (SDL_Colour){0,   200, 0,   255});
    digit_3   = make_digit(texture_data[2], (SDL_Colour){255, 0,   0,   255});
    digit_4   = make_digit(texture_data[3], (SDL_Colour){0,   0,   127, 255});
    digit_5   = make_digit(texture_data[4], (SDL_Colour){127, 0,   255, 255});
    digit_6   = make_digit(texture_data[5], (SDL_Colour){0,   127, 255, 255});
    mine_tex  = make_digit(texture_data[6], (SDL_Colour){0,   0,   0,   255});
    flag_tex  = make_digit(texture_data[7], (SDL_Colour){255, 0,   0,   255});
    tick_tex  = make_digit(texture_data[8], (SDL_Colour){0,   255, 0,   255});
    cross_tex = make_digit(texture_data[9], (SDL_Colour){255, 0,   0,   255});
}

static void init_ui(void)
{
    window = SDL_CreateWindow(
        "Minesweeper", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WINDOW_X,
        WINDOW_Y,
        SDL_WINDOW_SHOWN
    );
    renderer = SDL_CreateRenderer( 
        window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
}

static void fill_tile(Tile *t, SDL_Colour colour)
{
    Hex h = t->hex;
#define TRIANGLE(A, B, C) {A, colour, {0}}, {B, colour, {0}}, {C, colour, {0}},
    SDL_Vertex vertices[] = {
        TRIANGLE(h.a, h.b, h.f)
        TRIANGLE(h.b, h.c, h.f)
        TRIANGLE(h.f, h.c, h.e)
        TRIANGLE(h.d, h.e, h.c)
    };
#undef TRIANGLE
    SDL_RenderGeometry(renderer, NULL, vertices, 12, NULL, 0);
}

// Drawing

static void outline_tile(Tile *t, SDL_Colour colour)
{
    Hex h = t->hex;
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderDrawLine(renderer, h.a.x, h.a.y, h.b.x, h.b.y);
    SDL_RenderDrawLine(renderer, h.b.x, h.b.y, h.c.x, h.c.y);
    SDL_RenderDrawLine(renderer, h.c.x, h.c.y, h.d.x, h.d.y);
    SDL_RenderDrawLine(renderer, h.d.x, h.d.y, h.e.x, h.e.y);
    SDL_RenderDrawLine(renderer, h.e.x, h.e.y, h.f.x, h.f.y);
    SDL_RenderDrawLine(renderer, h.f.x, h.f.y, h.a.x, h.a.y);
}

static void draw_face(Tile *t, SDL_Texture *tex)
{
    SDL_RenderCopy(renderer, tex, NULL, &t->face);
}

static void draw_covered(Tile *t)
{
    fill_tile(t, TILE_COLOUR_1);
    outline_tile(t, TILE_COLOUR_2);
}

static void draw_uncovered(Tile *t)
{
    fill_tile(t, TILE_COLOUR_2);
    switch (t->type) {
    case TILE_MINE: draw_face(t, mine_tex); break;
    case TILE_1:    draw_face(t, digit_1);  break;
    case TILE_2:    draw_face(t, digit_2);  break;
    case TILE_3:    draw_face(t, digit_3);  break;
    case TILE_4:    draw_face(t, digit_4);  break;
    case TILE_5:    draw_face(t, digit_5);  break;
    case TILE_6:    draw_face(t, digit_6);  break;
    default: break;
    }
}

static void draw_tile(Tile *t)
{
    switch (t->cover) {
    case TILE_COVERED:
        draw_covered(t);
        break;
    case TILE_UNCOVERED:
        draw_uncovered(t);
        break;
    case TILE_FLAGGED:
        draw_covered(t);
        draw_face(t, flag_tex);
    }
}

static void draw_tile_game_over(Tile *t) 
{
    draw_uncovered(t);
    if (t->cover == TILE_FLAGGED) {
        if (t->type == TILE_MINE)
            draw_face(t, tick_tex);
        else
            draw_face(t, cross_tex);
    }
}

static void draw(void) 
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    SDL_RenderClear(renderer);
    for (int y = 0; y < TILES_Y; ++y) {
        for (int x = 0; x < row_max(y); ++x) {
            Tile *t = &tiles[x][y];
            if (game_over) {
                draw_tile_game_over(t);
            } else {
                draw_tile(t);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

// Game logic

static void end_game(const char *msg)
{
    puts(msg);
    game_over = true;
}

static void flag_tile(Tile *tile)
{
    if (tile->cover == TILE_COVERED) {
        tile->cover = TILE_FLAGGED;
        flags_placed++;
        if (tile->type == TILE_MINE) {
            flags_correct++;
        }
    } else if (tile->cover == TILE_FLAGGED) {
        tile->cover = TILE_COVERED;
        flags_placed--;
        if (tile->type == TILE_MINE) {
            flags_correct--;
        }
    }
    if (flags_placed == flags_correct && flags_correct == MINE_COUNT)
        end_game("Win");
}

static void uncover_tile(Tile *tile) 
{
    if (tile->cover == TILE_FLAGGED)
        return;
    if (first_click) {
        while (tile->type != TILE_EMPTY) {
            for (int y = 0; y < TILES_Y; ++y) {
                for (int x = 0; x < row_max(y); ++x) {
                    get_tile(x, y)->type = TILE_EMPTY;
                }
            }
            init_mines();
        }
        first_click = false;
    } else if (tile->type == TILE_MINE) {
        end_game("Lose");
        return;
    }
    Tile *visited[255];
    Tile *agenda[255] = {tile};
    int visited_len = 0;
    int agenda_len = 1;
    while (agenda_len > 0) {
        Tile *tile = agenda[--agenda_len];
        visited[visited_len++] = tile;
        if (tile->cover != TILE_COVERED) {
            continue;
        }
        tile->cover = TILE_UNCOVERED;
        if (tile->type != TILE_EMPTY) {
            continue;
        }
        Tile *neighbours[6];
        get_neighbours(tile, neighbours);
        for (int i = 0; i < 6; ++i) {
            Tile *tile = neighbours[i];
            bool has_been_visited = false;
            for (int j = 0; j < visited_len; ++j) {
                if (tile == visited[j]) {
                    has_been_visited = true;
                    break;
                }
            }
            if (!has_been_visited)
                agenda[agenda_len++] = tile;
        }
    }
}

static void handle_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }
            break;
        case SDL_MOUSEBUTTONDOWN: {
            if (game_over)
                break;
            int x, y;
            screen_to_world(event.button.x, event.button.y, &x, &y);
            Tile *tile = get_tile(x, y);
            if (event.button.button == SDL_BUTTON_RIGHT) {
                flag_tile(tile); 
            } else if (event.button.button == SDL_BUTTON_LEFT) {
                uncover_tile(tile);
            }
        }}
    }
}

int main(void)
{
    random_state = (int) time(NULL);
    init_ui();
    init_vertices();
    init_mines();
    init_textures();
    while (running) {
        handle_events();
        draw();
    }
    return 0;
}

