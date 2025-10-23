#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_thread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "audio_manager.h"

#define SDL_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO)
#define WINDOW_TITLE "Conway's Game of Life | Playing"
#define WINDOW_WIDTH 1050
#define WINDOW_HEIGHT 945
#define TILE_SIZE 35
#define GRID_WIDTH (WINDOW_WIDTH / TILE_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / TILE_SIZE)

int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
int next_grid[GRID_HEIGHT][GRID_WIDTH] = {0};

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    bool is_running, is_playing;
    int update_freq;
    int tile_color[4];
};

void show_menu_window(char window_title[], int win_x, int win_y, char *lines[], int num_lines) {
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }

    SDL_Window *win = SDL_CreateWindow(window_title, win_x, win_y, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 20);
    if (!font) {
        fprintf(stderr, "Error loading font: %s\n", SDL_GetError());
        return;
    }

    SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    SDL_RenderClear(ren);
    SDL_Color white = {255, 255, 255, 255};
    int y = 30;
    for (int i = 0; i < num_lines; i++) {
        SDL_Surface *surf = TTF_RenderText_Solid(font, lines[i], strlen(lines[i]), white);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FRect dst = {30, y, surf->w, surf->h};
        SDL_RenderTexture(ren, tex, NULL, &dst);
        
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surf);

        y += 40;
    }

    SDL_RenderPresent(ren);

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (e.window.windowID == SDL_GetWindowID(win)) running = false;
            }
        }
        SDL_Delay(50);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
}

void show_help_window() {

    char *lines[] = {
        "Hotkeys:",
        "[Space] - Play / Pause",
        "[C] - Clear grid",
        "[G] - Randomize grid",
        "[Mouse] - Toggle cell",
        "[N] - Next generation",
        "[UP] - Speed up simulation",
        "[DOWN] - Slow down simulation",
        "[P] - Show Patterns menu",
        "[H] - Show this help menu",
        "[S] - Customize simulation",
        "[ESC] - Quit"
    };

    
    int num_lines = sizeof(lines) / sizeof(lines[0]);

    show_menu_window("Conway's Game of Life | Hotkeys", 600, 600, lines, num_lines);

}

void show_patterns_window() {

    char *lines[] = {
        "Preloaded Patterns:",
        "[1] - Glider",
        "[2] - Blinker",
        "[3] - Gosper Glider Gun"
    };

    int num_lines = sizeof(lines) / sizeof(lines[0]);

    show_menu_window("Conway's Game of Life | Preloaded Patterns", 600, 400, lines, num_lines);

}

bool game_init_sdl(struct Game *g) {
    if (SDL_Init(SDL_FLAGS) < 0) {
        fprintf(stderr, "Error initializing SDL3: %s\n", SDL_GetError());
        return false;
    }

    g -> window = SDL_CreateWindow(
        WINDOW_TITLE,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    if (!g -> window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return false;
    }

    g -> renderer = SDL_CreateRenderer(g -> window, NULL);
    if (!g -> renderer) {
        fprintf(stderr, "Error creating render: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool game_new(struct Game *g) {
    if (!game_init_sdl(g)) {
        return false;
    }

    if (!init_audio_system()) {
        SDL_Log("Failed to initialize audio system: %s\n", SDL_GetError());
        return false;
    }
    if (!play_background_music("assets/background.wav")) {
        SDL_Log("Failed to start background music: %s\n", SDL_GetError());
        return false;
    }
    
    g -> is_running = true;
    g -> is_playing = true;
    g -> update_freq = 60;
    g -> tile_color[0] = 255;
    g -> tile_color[1] = 255;
    g -> tile_color[2] = 0;
    g -> tile_color[3] = 255;
    return true;
}

void game_free(struct Game *g) {
    if (g -> renderer) {
        SDL_DestroyRenderer(g -> renderer);
        g -> renderer = NULL;
    }

    if (g -> window) {
        SDL_DestroyWindow(g -> window);
        g -> window = NULL;
    }
    
    stop_background_music();
    shutdown_audio_system();
    SDL_Quit();
}

void grid_randomize() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = rand() % 2;
        }
    }
}

int count_neighbours(int y, int x) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        if (y+dy < 0 || y+dy >= GRID_HEIGHT) continue;
        for (int dx = -1; dx <= 1; dx++) {
            if (x+dx < 0 || x+dx >= GRID_WIDTH) continue;
            if (dx == 0 && dy == 0) continue;
            count += grid[y+dy][x+dx];
            
        }
    }
    return count;
}

void update_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int neighbours = count_neighbours(y,x);
            if (grid[y][x]) {
                next_grid[y][x] = (neighbours == 2 || neighbours == 3);
            } else {
                next_grid[y][x] = (neighbours == 3);
            }
        }
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = next_grid[y][x];
        }
    }
}

void clear_screen() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
}

void load_rle(const char* filename, int offset_y, int offset_x, bool clear_before) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error loading rle: %s\n", filename);
        return;
    }
    char line[1024];
    int pattern_w = -1, pattern_h = -1;
    bool header_found = false;

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char) *p)) p++;
        if (*p == '#') continue;
        if (strstr(p, "x") && strstr(p, "=") && strstr(p, "y")) {
            header_found = true;
            if (sscanf(p, "x = %d, y = %d", &pattern_w, &pattern_h) < 2) {
                sscanf(p, "x = %d, y = %d", &pattern_w, &pattern_h);
            }
            break;
        }
    }

    if (!header_found) {
        rewind(f);
    }

    if (clear_before) {
        clear_screen();
    }

    int cur_x = 0, cur_y = 0;
    int run = 0;
    bool done = false;

    while (!done && fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p) {
            if (isdigit((unsigned char) *p)) {
                run = run * 10 + (*p - '0');
            } else if (*p == 'o' || *p == 'O') {
                if (run == 0) run = 1;
                for (int i = 0; i < run; i++) {
                    int gx = offset_x + cur_x;
                    int gy = offset_y + cur_y;
                    if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {
                        grid[gy][gx] = 1;
                    }
                    cur_x++;
                }
                run = 0;
            } else if (*p == 'b' || *p == '.') {
                if (run == 0) run = 1;
                cur_x += run;
                run = 0;
            } else if (*p == '$') {
                if (run == 0) run = 1;
                cur_y += run;
                cur_x = 0;
                run = 0;
            } else if (*p == '!') {
                done = true;
                break;
            }
            p++;
        }
    }

    fclose(f);
    fprintf(stdout, "Loaded RLE: '%s'\n", filename);

}

struct Color {
    Uint8 r, g, b, a;
};

static void draw_slider(SDL_Renderer *ren, int x, int y, int value, SDL_Color barColor) {
    float width = 200.0f;
    SDL_FRect track = {x, y, width, 10};
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
    SDL_RenderFillRect(ren, &track);

    float scaled_value = (value / 255.0f) * width;

    SDL_FRect fill = {x, y, scaled_value, 10};
    SDL_SetRenderDrawColor(ren, barColor.r, barColor.g, barColor.b, barColor.a);
    SDL_RenderFillRect(ren, &fill);

    SDL_FRect knob = {x + scaled_value - 5, y - 3, 10, 16};
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderFillRect(ren, &knob);
}

struct Color open_color_slider_picker(struct Game *g) {
    SDL_Window *win = SDL_CreateWindow("Color Picker", 600, 300, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    struct Color current = {g -> tile_color[0], g -> tile_color[1], g -> tile_color[2], g -> tile_color[3]};
    bool running = true, dragging = false;
    int activeSlider = -1;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (e.window.windowID == SDL_GetWindowID(win)) running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    dragging = true;
                    {
                        int mx = e.button.x, my = e.button.y;
                        if (my >= 60 && my <= 75) activeSlider = 0;
                        else if (my >= 100 && my <= 115) activeSlider = 1;
                        else if (my >= 140 && my <= 155) activeSlider = 2;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    dragging = false;
                    activeSlider = -1;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (dragging && activeSlider != -1) {
                        int mx = e.motion.x;
                        int value = ((mx - 100) / 200.0f) * 255;
                        if (value < 0) value = 0;
                        if (value > 255) value = 255;

                        if (activeSlider == 0) current.r = value;
                        if (activeSlider == 1) current.g = value;
                        if (activeSlider == 2) current.b = value;
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (e.key.scancode == SDL_SCANCODE_RETURN) running = false;
                    break;
                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);
        
        SDL_Color red = {255, 0, 0, 255};
        SDL_Color green = {0, 255, 0, 255};
        SDL_Color blue = {0, 0, 255, 255};

        draw_slider(ren, 100, 60, current.r, red);
        draw_slider(ren, 100, 100, current.g, green);
        draw_slider(ren, 100, 140, current.b, blue);

        SDL_FRect preview = {130, 190, 140, 70};
        SDL_SetRenderDrawColor(ren, current.r, current.g, current.b, 255);
        SDL_RenderFillRect(ren, &preview);

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    return current;
}

void customize_game(struct Game *g) {
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }

    SDL_Window *cust_win = SDL_CreateWindow("Conway's Game of Life | Customize Game", 600, 600, 0);
    SDL_Renderer *cust_ren = SDL_CreateRenderer(cust_win, NULL);

    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 20);
    if (!font) {
        SDL_Log("Error loading font: %s\n", SDL_GetError());
        return;
    }

    const char *lines[] = {
        "Customizations for tile color:",
        "[1] - Yellow",
        "[2] - Blue",
        "[3] - Green",
        "[4] - Red",
        "[5] - Orange",
        "[6] - White",
        "[C] - Customize (choose)"
    };

    int num_lines = sizeof(lines) / sizeof(lines[0]);

    SDL_SetRenderDrawColor(cust_ren, 40, 40, 40, 255);
    SDL_RenderClear(cust_ren);

    SDL_Color white = {255, 255, 255, 255};
    int y = 30;
    for (int i = 0; i < num_lines; i++) {
        SDL_Surface *surf = TTF_RenderText_Solid(font, lines[i], strlen(lines[i]), white);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(cust_ren, surf);
        SDL_FRect dst = {30, y, surf->w, surf->h};
        SDL_RenderTexture(cust_ren, tex, NULL, &dst);

        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surf);

        y += 40;
    }

    SDL_RenderPresent(cust_ren);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (e.window.windowID == SDL_GetWindowID(cust_win)) running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (e.key.scancode) {
                        case SDL_SCANCODE_1:
                            g -> tile_color[0] = 255;
                            g -> tile_color[1] = 255;
                            g -> tile_color[2] = 0;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_2:
                            g -> tile_color[0] = 0;
                            g -> tile_color[1] = 0;
                            g -> tile_color[2] = 255;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_3:
                            g -> tile_color[0] = 0;
                            g -> tile_color[1] = 255;
                            g -> tile_color[2] = 0;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_4:
                            g -> tile_color[0] = 255;
                            g -> tile_color[1] = 0;
                            g -> tile_color[2] = 0;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_5:
                            g -> tile_color[0] = 255;
                            g -> tile_color[1] = 165;
                            g -> tile_color[2] = 0;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_6:
                            g -> tile_color[0] = 255;
                            g -> tile_color[1] = 255;
                            g -> tile_color[2] = 255;
                            g -> tile_color[3] = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_C:
                            running = false;
                            struct Color chosen = open_color_slider_picker(g);
                            g -> tile_color[0] = chosen.r;
                            g -> tile_color[1] = chosen.g;
                            g -> tile_color[2] = chosen.b;
                            g -> tile_color[3] = chosen.a;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        SDL_Delay(50);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(cust_ren);
    SDL_DestroyWindow(cust_win);
    TTF_Quit();
}

struct PatternOptions {
    int offset_x;
    int offset_y;
    bool clear;
    bool confirmed;
};

void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, text, strlen(text), color);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FRect dst = {x, y, surf->w, surf->h};
    SDL_RenderTexture(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_DestroySurface(surf);
}

void draw_button(SDL_Renderer *ren, TTF_Font *font, SDL_FRect rect, const char *text, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, 0, 200, 0, 255);
    SDL_RenderFillRect(ren, &rect);
    draw_text(ren, font, text, rect.x + 10, rect.y + 10, color);
}

void customize_preloaded_pattern(const char* filename, char pattern_name[]) {
    struct PatternOptions opts = {0, 0, false, false};

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }

    SDL_Window *win = SDL_CreateWindow("Conway's Game of Life | Pattern Options", 600, 400, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 20);
    if (!font) {
        SDL_Log("Error loading font: %s\n", SDL_GetError());
        return;
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (e.window.windowID == SDL_GetWindowID(win)) running = false;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int mx = e.button.x;
                int my = e.button.y;

                if (mx > 250 && mx < 270 && my > 80 && my < 100 && opts.offset_x < 29) {
                    opts.offset_x++;
                }

                if (mx > 200 && mx < 220 && my > 80 && my < 100 && opts.offset_x > 0) {
                    opts.offset_x--;
                }

                if (mx > 250 && mx < 270 && my > 120 && my < 140 && opts.offset_y < 26) {
                    opts.offset_y++;
                }

                if (mx > 200 && mx < 220 && my > 120 && my < 140 && opts.offset_y > 0) {
                    opts.offset_y--;
                }

                if (mx > 50 && mx < 70 && my > 170 && my < 190) {
                    opts.clear = !opts.clear;
                }

                if (mx > 80 && mx < 160 && my > 220 && my < 260) {
                    opts.confirmed = true;
                    running = false;
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);

        SDL_Color white = {255, 255, 255, 255};

        char buf[128];
        snprintf(buf, sizeof(buf), "Options for %s", pattern_name);
        draw_text(ren, font, buf, 20, 20, white);

        snprintf(buf, sizeof(buf), "X Offset: %d", opts.offset_x);
        draw_text(ren, font, buf, 80, 80, white);

        SDL_FRect plusX = {250, 80, 20, 20};
        SDL_FRect minusX = {200, 80, 20, 20};
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        SDL_RenderFillRect(ren, &plusX);
        SDL_RenderFillRect(ren, &minusX);
        draw_text(ren, font, "+", 252, 78, white);
        draw_text(ren, font, "-", 206, 78, white);

        snprintf(buf, sizeof(buf), "Y Offset: %d", opts.offset_y);
        draw_text(ren, font, buf, 80, 120, white);

        SDL_FRect plusY = {250, 120, 20, 20};
        SDL_FRect minusY = {200, 120, 20, 20};
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        SDL_RenderFillRect(ren, &plusY);
        SDL_RenderFillRect(ren, &minusY);
        draw_text(ren, font, "+", 252, 118, white);
        draw_text(ren, font, "-", 206, 118, white);

        SDL_FRect checkbox = {50, 170, 20, 20};
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderRect(ren, &checkbox);
        if (opts.clear) {
            // SDL_RenderLine(ren, 50, 170, 70, 170);
            // SDL_RenderLine(ren, 70, 170, 50, 170);
            SDL_FRect fillCheckbox = {50, 170, 20, 20};
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            SDL_RenderFillRect(ren, &fillCheckbox);
        }
        draw_text(ren, font, "Clear screen first", 80, 170, white);

        SDL_FRect apply = {80, 220, 80, 40};
        draw_button(ren, font, apply, "Apply", white);

        SDL_RenderPresent(ren);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();

    if (opts.confirmed) {
        printf("Loading pattern...\n");
        load_rle(filename, opts.offset_y, opts.offset_x, opts.clear);
    }

}

void game_events(struct Game *g) {
    while (SDL_PollEvent(&g->event)) {
        switch (g->event.type) {
            case SDL_EVENT_QUIT:
                g->is_running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (g->event.key.scancode) {
                    case SDL_SCANCODE_ESCAPE:
                        g->is_running = false;
                        break;
                    case SDL_SCANCODE_SPACE:
                        g->is_playing = !g->is_playing;
                        if (g->is_playing) {
                            resume_background_music();
                        } else {
                            pause_background_music();
                        }
                        break;
                    case SDL_SCANCODE_C:
                        clear_screen();
                        g->is_playing = false;
                        pause_background_music();
                        play_sfx("assets/clear.wav");
                        break;
                    case SDL_SCANCODE_G:
                        grid_randomize();
                        play_sfx("assets/randomize.wav");
                        break;
                    case SDL_SCANCODE_N:
                        if (!g->is_playing) {
                            update_grid();
                            play_sfx("assets/next_gen.wav");
                        }
                        break;
                    case SDL_SCANCODE_H:
                        show_help_window();
                        break;
                    case SDL_SCANCODE_P:
                        show_patterns_window();
                        break;
                    case SDL_SCANCODE_S:
                        customize_game(g);
                        break;
                    case SDL_SCANCODE_1:
                        customize_preloaded_pattern("patterns/glider.rle", "Glider");
                        break;
                    case SDL_SCANCODE_2:
                        customize_preloaded_pattern("patterns/blinker.rle", "Blinker");
                        break;
                    case SDL_SCANCODE_3:
                        customize_preloaded_pattern("patterns/gosper_glider_gun.rle", "Gosper Glider Gun");
                        break;
                    case SDL_SCANCODE_UP:
                        if (g->update_freq > 1) g->update_freq--;
                        break;
                    case SDL_SCANCODE_DOWN:
                        g->update_freq++;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                SDL_MouseButtonEvent *mouseButtonEvent = (SDL_MouseButtonEvent*) &g->event;
                int mouseX = (int) mouseButtonEvent -> x;
                int mouseY = (int) mouseButtonEvent -> y;
                int x_g = mouseX / TILE_SIZE;
                int y_g = mouseY / TILE_SIZE;
                grid[y_g][x_g] = !(grid[y_g][x_g]);
                play_sfx("assets/toggle.wav");
                break;
            }
            default:
                break;
        }
    }
}

void draw_grid_lines(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, 255, 255, 255, 255);
    
    for (int x = 0; x < WINDOW_WIDTH; x+=TILE_SIZE) {
        SDL_RenderLine(g->renderer, x, 0, x, WINDOW_HEIGHT);
    }
    for (int y = 0; y < WINDOW_HEIGHT; y+=TILE_SIZE) {
        SDL_RenderLine(g->renderer, 0, y, WINDOW_WIDTH, y);
    }
}

void draw_grid(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, g->tile_color[0], g->tile_color[1], g->tile_color[2], g->tile_color[3]);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x]) {
                SDL_FRect rect = {
                    .x = x * TILE_SIZE,
                    .y = y * TILE_SIZE,
                    .w = TILE_SIZE,
                    .h = TILE_SIZE
                };
                SDL_RenderFillRect(g->renderer, &rect);
            }
        }
    }
}

void game_draw(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g->renderer);
    draw_grid(g);
    draw_grid_lines(g);
    SDL_RenderPresent(g->renderer);
} 

void game_run(struct Game *g) {
    int count = 0;


    while (g -> is_running) {
        if (g -> is_playing) {
            count++;
        }
        
        if (count >= g -> update_freq) {
            count = 0;
            update_grid();
        }

        SDL_SetWindowTitle(g->window, g->is_playing? "Conway's Game of Life | Playing" : "Conway's Game of Life | Paused");
        game_events(g);
        game_draw(g);
        SDL_Delay(16);
    }
}

int main(int argc, char *argv[]) {
    bool exit_status = EXIT_FAILURE;

    struct Game game = {0};

    if (game_new(&game)) {
        game_run(&game);
        exit_status = EXIT_SUCCESS;
    }

    game_free(&game);

    return exit_status;
}