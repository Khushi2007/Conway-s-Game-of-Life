/**
 * @file main.c
 * @brief Conway's Game of Life implementation using SDL3 for rendering and audio.
 * 
 * This file contains the main game logic, rendering routines, event handling, and integration
 * with the audio manager. It provides an interactive simulation of Conway's Game of Life featuring
 * background music, sound effects, various user controls and customization options.
 */

#include <SDL3/SDL.h> // for SDL main functionalities
#include <SDL3/SDL_main.h> // for SDL main entry point
#include <SDL3_ttf/SDL_ttf.h> // for SDL TTF font rendering
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "audio_manager.h" // for audio functionalities

/* --------------------------------------------------------------------------------------------
 * Game Configuration Constants
 * -------------------------------------------------------------------------------------------- */

#define SDL_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO) // SDL initialization flags
#define WINDOW_TITLE "Conway's Game of Life | Playing" // Window title
#define WINDOW_WIDTH 1050 // Window width in pixels
#define WINDOW_HEIGHT 945 // Window height in pixels
#define TILE_SIZE 35 // Size of each grid tile in pixels
#define GRID_WIDTH (WINDOW_WIDTH / TILE_SIZE) // Number of tiles horizontally
#define GRID_HEIGHT (WINDOW_HEIGHT / TILE_SIZE) // Number of tiles vertically

/* --------------------------------------------------------------------------------------------
 * Global Grid Arrays
 * --------------------------------------------------------------------------------------------
 * `grid` holds the current state of the cells; `next_grid` is used for computing the next state
 * before swapping.
 * -------------------------------------------------------------------------------------------- */

int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
int next_grid[GRID_HEIGHT][GRID_WIDTH] = {0};

/* --------------------------------------------------------------------------------------------
 * Struct Definitions
 * -------------------------------------------------------------------------------------------- */

/**
 * @struct Color
 * @brief Represents an RGBA color.
 */

// Parv and Omkumar
struct Color {
    Uint8 r, g, b, a;
};

/**
 * @struct Game
 * @brief Holds the main runtime state of the game including SDL components and game flags.
 */

// Vanshi and Khushi, Het and Virat
struct Game {
    SDL_Window *window; // Main application window
    SDL_Renderer *renderer; // Renderer for drawing tiles and UI
    SDL_Event event; // Global event handler
    bool is_running; // True if game is active
    bool is_playing; // True if simulation is running (not paused)
    int update_freq; // Simulation update delay in milliseconds (speed control)
    struct Color tile_color; // RGBA color for live cellsd
};

/**
 * @struct PatternOptions
 * @brief Stores customization options for loading pre-defined patterns in the Game of Life.
 * 
 * This structure encapsulates user-defined parameters that control how a pattern
 * (loaded from an RLE file) is positioned and applied to the main simulation grid.
 * It is primarily used in the @ref customize_preloaded_pattern() function.
 */

// Harmit and Yuvraj
struct PatternOptions {
    int offset_x; // Horizontal offset from left edge of grid where the pattern should be placed
    int offset_y; // Vertical offset from top of grid where the pattern should be placed
    bool clear; // Boolean flag indicating whether to clear the grid before loading new pattern
    bool confirmed; // Boolean flag indicating whether the user has confirmed their choices
};

/* --------------------------------------------------------------------------------------------
 * Utility Windows (Help, Patterns, Menus, Color Picker)
 * --------------------------------------------------------------------------------------------
 * Helper functions to display various informational and configuration windows.
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Renders a simple window displaying multiple lines of text.
 * @param window_title Title of the window.
 * @param win_x Width of the window.
 * @param win_y Height of the window.
 * @param lines Array of text lines to display.
 * @param num_lines Number of text lines.
 */

// Vanshi and Khushi
void show_menu_window(char window_title[], int win_x, int win_y, char *lines[], int num_lines) {
    // TTF must be initialized before calling this function
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }
    
    // Create menu window and renderer
    SDL_Window *win = SDL_CreateWindow(window_title, win_x, win_y, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    // Load font for rendering text
    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 20);
    if (!font) {
        fprintf(stderr, "Error loading font: %s\n", SDL_GetError());
        return;
    }

    // Color the background gray and render each line of text
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
    // Present the rendered content
    SDL_RenderPresent(ren);

    // Event loop to keep window open until closed by user
    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (e.window.windowID == SDL_GetWindowID(win)) running = false;
            }
        }
        SDL_Delay(50); // Delay to reduce CPU usage
    }

    // Cleanup resources
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
}

/**
 * @brief Displays the help window listing available hotkeys.
 */

// Het and Virat
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

/**
 * @brief Displays the preloaded patterns window.
 */

// Harmit and Yuvraj
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

/* --------------------------------------------------------------------------------------------
 * SDL Initialization and Game Lifecycle
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Initializes SDL3 video and renderer subsystems.
 * @param g Pointer to the Game instance.
 * @return true if initialization is successful, false otherwise.
 */

// Vanshi and Khushi
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

/**
 * @brief Initializes the game state and audio system.
 * @param g Pointer to the Game instance.
 * @return true if game setup is successful, false otherwise.
 */

// Vanshi and Khushi
bool game_new(struct Game *g) {
    // Initialize SDL subsystems
    if (!game_init_sdl(g)) {
        return false;
    }
    // Initialize audio system and start background music
    if (!init_audio_system()) {
        SDL_Log("Failed to initialize audio system: %s\n", SDL_GetError());
        return false;
    }
    if (!play_background_music("assets/background.wav")) {
        SDL_Log("Failed to start background music: %s\n", SDL_GetError());
        return false;
    }
    
    // Set initial game state
    g -> is_running = true;
    g -> is_playing = true;
    g -> update_freq = 60;
    g -> tile_color.r = 255;
    g -> tile_color.g = 255;
    g -> tile_color.b = 0;
    g -> tile_color.a = 255;
    return true;
}

/**
 * @brief Frees all allocated resources and shuts down SDL and audio systems.
 * @param g Pointer to the Game instance.
 */

// Vanshi and Khushi
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

/* --------------------------------------------------------------------------------------------
 * Grid Simulation Logic
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Randomizes the grid with live and dead cells.
 */

// Het and Virat
void grid_randomize() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = rand() % 2; // Randomly assign 0 or 1
        }
    }
}

/**
 * @brief Counts the number of live neighbours for a given cell.
 * @param y Y-coordinate of the cell.
 * @param x X-coordinate of the cell.
 * @return Number of live neighbouring cells.
 */

// Prateek and Hunar
int count_neighbours(int y, int x) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        if (y+dy < 0 || y+dy >= GRID_HEIGHT) continue; // Boundary check
        for (int dx = -1; dx <= 1; dx++) {
            if (x+dx < 0 || x+dx >= GRID_WIDTH) continue; // Boundary check
            if (dx == 0 && dy == 0) continue; // Skip the cell itself
            count += grid[y+dy][x+dx];
            
        }
    }
    return count;
}

/**
 * @brief Computes and applies the next generation of the grid based on Conway's rules.
 */

// Prateek and Hunar
void update_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int neighbours = count_neighbours(y,x);
            if (grid[y][x]) {
                next_grid[y][x] = (neighbours == 2 || neighbours == 3); // Survival
            } else {
                next_grid[y][x] = (neighbours == 3); // Birth
            }
        }
    }

    // Copy the next grid to the current grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = next_grid[y][x];
        }
    }
}

/**
 * @brief Clears all live cells from the grid.
 */

// Het and Virat
void clear_screen() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
}

/* --------------------------------------------------------------------------------------------
 * Pre-loaded Patterns 
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Loads a pre-saved RLE pattern into the grid.
 * @param filename Path to RLE file.
 * @param offset_y Y-offset to apply when drawing the pattern.
 * @param offset_x X-offset to apply when drawing the pattern.
 * @param clear_before Whether to clear the grid before loading.
 */

// Harmit and Yuvraj
void load_rle(const char* filename, int offset_y, int offset_x, bool clear_before) {
    FILE *f = fopen(filename, "r");
    // Check if file opened successfully
    if (!f) {
        fprintf(stderr, "Error loading rle: %s\n", filename);
        return;
    }
    // Read header to get pattern dimensions (if present)
    char line[1024];
    int pattern_w = -1, pattern_h = -1;
    bool header_found = false;

    while (fgets(line, sizeof(line), f)) {
        // Trim whitespace
        char *p = line;
        while (isspace((unsigned char) *p)) p++;
        if (*p == '#') continue; // Skip comments
        // Look for dimension line
        if (strstr(p, "x") && strstr(p, "=") && strstr(p, "y")) {
            header_found = true;
            if (sscanf(p, "x = %d, y = %d", &pattern_w, &pattern_h) < 2) {
                sscanf(p, "x = %d, y = %d", &pattern_w, &pattern_h);
            }
            break;
        }
    }
    // If no header found, rewind to start of the file
    if (!header_found) {
        rewind(f);
    }

    if (clear_before) {
        clear_screen();
    }

    // Parse RLE data
    int cur_x = 0, cur_y = 0; // Current position in the grid
    int run = 0; // Length of current run
    bool done = false; // Flag to indicate end of pattern

    // Read RLE data
    while (!done && fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p) {
            if (isdigit((unsigned char) *p)) { // Run length
                run = run * 10 + (*p - '0');
            } else if (*p == 'o' || *p == 'O') { // Live cells
                if (run == 0) run = 1;
                for (int i = 0; i < run; i++) {
                    int gx = offset_x + cur_x;
                    int gy = offset_y + cur_y;
                    // Set cell to alive if within bounds
                    if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {
                        grid[gy][gx] = 1;
                    }
                    cur_x++; // Move to next cell
                }
                run = 0; // Reset run length
            } else if (*p == 'b' || *p == '.') { // Dead cells
                if (run == 0) run = 1;
                cur_x += run; // Skip dead cells
                run = 0; // Reset run length
            } else if (*p == '$') { // New line
                if (run == 0) run = 1;
                cur_y += run; // Move down by run lines
                cur_x = 0; // Reset to start of line
                run = 0; // Reset run length
            } else if (*p == '!') { // End of pattern
                done = true;
                break;
            }
            p++; // Move to next character
        }
    }
    
    fclose(f);
    fprintf(stdout, "Loaded RLE: '%s'\n", filename);

}

/* --------------------------------------------------------------------------------------------
 * Color Picker and Slider System
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Draws a horizontal color slider used in the color picker interface.
 * 
 * This function renders a slider composed of three visual elements:
 * a dark gray track,
 * a colored fill portion indicating the current value,
 * and a white knob representing the slider handle.
 * 
 * The slider visually represents a numeric value between 0 and 255
 * scaled to fit within a fixed width of 200 pixels.
 * 
 * @param ren The SDL_Renderer used for drawing the slider.
 * @param x The X-coordinate of the slider's starting position.
 * @param y The Y-coordinate of the slider's starting position.
 * @param value The current slider value (expected range: 0-255).
 * @param barColor The SDL_Color determining the color of the slider's filled portion.
 * 
 * @note This function does not handle user input; it only draws the slider.
 * Input handling (dragging and updating `value`) is implemented separately.
 */

// Vanshi and Khushi
static void draw_slider(SDL_Renderer *ren, int x, int y, int value, SDL_Color barColor) {
    float width = 200.0f; // Fixed width of the slider
    SDL_FRect track = {x, y, width, 10}; // Slider track rectangle
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255); // Dark gray color for track
    SDL_RenderFillRect(ren, &track); // Draw track

    float scaled_value = (value / 255.0f) * width; // Scale value to slider width

    SDL_FRect fill = {x, y, scaled_value, 10}; // Filled portion rectangle
    SDL_SetRenderDrawColor(ren, barColor.r, barColor.g, barColor.b, barColor.a); // Set fill color
    SDL_RenderFillRect(ren, &fill); // Draw filled portion

    SDL_FRect knob = {x + scaled_value - 5, y - 3, 10, 16};
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderFillRect(ren, &knob);
}

/**
 * @brief Opens a color picker window with RGB sliders for live customization.
 * 
 * This function creates an SDL_Window containing three horizontal sliders
 * - one each for Red, Green and Blue channels - allowing the user to interactively choose a color.
 * The selected color is previewed in a rectangular swatch below the sliders. The user can adjust
 * sliders using mouse dragging, and press **Enter** to confirm and close the window.
 * 
 * @param g Pointer to the Game structure that holds the current tile color.
 *          The initial color of the sliders is set to the current tile color parameters.
 * 
 * @return A `struct Color` representing the final RGB (and alpha) values selected by the user
 *         before closing the window.
 * 
 * @details
 * - The sliders range from 0-255 for each color component.
 * - The preview box dynamically updates as the user drags the sliders.
 * - The function blocks until the window is closed or **Enter** is pressed.
 * 
 * @note
 * This function only provides a user interface for selecting colors. It does not directly apply
 * the chosen color to the game. The returned color is manually assigned to the tile color later.
 */

// Vanshi and Khushi
struct Color open_color_slider_picker(struct Game *g) {
    // Create SDL window and renderer for color picker
    SDL_Window *win = SDL_CreateWindow("Color Picker", 600, 300, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    // Initialize current color from game's tile color
    struct Color current = {g -> tile_color.r, g -> tile_color.g, g -> tile_color.b, g -> tile_color.a};
    // Make a copy to revert if needed on cancel
    struct Color current_cpy = {g -> tile_color.r, g -> tile_color.g, g -> tile_color.b, g -> tile_color.a};
    // Event loop variables
    bool running = true, dragging = false;
    // Active slider index: 0 - Red, 1 - Green, 2 - Blue, -1 - none
    int activeSlider = -1;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (e.window.windowID == SDL_GetWindowID(win)) {
                        // Revert to original color on window close without confirmation
                        current.r = current_cpy.r;
                        current.g = current_cpy.g;
                        current.b = current_cpy.b;
                        current.a = current_cpy.a;
                        running = false;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    dragging = true; // Start dragging
                    {
                        // Determine which slider is active based on mouse Y position
                        int mx = e.button.x, my = e.button.y;
                        if (my >= 60 && my <= 75) activeSlider = 0;
                        else if (my >= 100 && my <= 115) activeSlider = 1;
                        else if (my >= 140 && my <= 155) activeSlider = 2;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    dragging = false; // Stop dragging
                    activeSlider = -1;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    // Update color value based on mouse X position while dragging
                    if (dragging && activeSlider != -1) {
                        int mx = e.motion.x;
                        int value = ((mx - 100) / 200.0f) * 255; // Map mouse X to 0-255 range
                        if (value < 0) value = 0; // Clamp value
                        if (value > 255) value = 255; // Clamp value

                        if (activeSlider == 0) current.r = value; // Red slider
                        if (activeSlider == 1) current.g = value; // Green slider
                        if (activeSlider == 2) current.b = value; // Blue slider
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (e.key.scancode == SDL_SCANCODE_RETURN) running = false;
                    break;
                default:
                    break;
            }
        }
        
        // Render the color picker interface
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);
        
        SDL_Color red = {255, 0, 0, 255};
        SDL_Color green = {0, 255, 0, 255};
        SDL_Color blue = {0, 0, 255, 255};

        // Draw color sliders
        draw_slider(ren, 100, 60, current.r, red);
        draw_slider(ren, 100, 100, current.g, green);
        draw_slider(ren, 100, 140, current.b, blue);

        // Draw color preview box
        SDL_FRect preview = {130, 190, 140, 70};
        SDL_SetRenderDrawColor(ren, current.r, current.g, current.b, 255);
        SDL_RenderFillRect(ren, &preview);

        SDL_RenderPresent(ren);
    }

    // Cleanup and return the selected color
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    return current;
}

/**
 * @brief Opens the customization window for selecting tile colors in the game.
 * 
 * This function displays a dedicated SDL_Window allowing the user to choose a new tile color for
 * the Game of Life grid. The interface offers several preset color options and a "Customize"
 * option that opens a detailed RGB slider-based color picker window.
 * 
 * @param g Pointer to the active `Game` structure whose tile color settings will be updated
 *          based on the user's selection.
 * 
 * @details
 * - The customization menu is rendered with labeled color options numbered 1-6
 * - Pressing a number key applies the corresponding color immediately.
 * - Pressing **C** launches the interactive color picker window for fine-grained control.
 * - Pressing the window close button exits without applying changes.
 * - The chosen color is stored in the `tile_color` array of the `Game` struct.
 * 
 * @note
 * The function blocks execution until the user makes a selection or closes the window.
 * It uses SDL_ttf for text rendering and must be called after SDL and TTF are initialized.
 */

// Vanshi and Khushi, Parv and Omkumar
void customize_game(struct Game *g) {
    // Initialize TTF for text rendering
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }

    // Create customization window and renderer
    SDL_Window *cust_win = SDL_CreateWindow("Conway's Game of Life | Customize Game", 600, 600, 0);
    SDL_Renderer *cust_ren = SDL_CreateRenderer(cust_win, NULL);

    // Load font for rendering text
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

    // Render the customization menu
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

    // Event loop to handle user input for color selection
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
                            g -> tile_color.r = 255;
                            g -> tile_color.g = 255;
                            g -> tile_color.b = 0;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_2:
                            g -> tile_color.r = 0;
                            g -> tile_color.g = 0;
                            g -> tile_color.b = 255;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_3:
                            g -> tile_color.r = 0;
                            g -> tile_color.g = 255;
                            g -> tile_color.b = 0;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_4:
                            g -> tile_color.r = 255;
                            g -> tile_color.g = 0;
                            g -> tile_color.b = 0;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_5:
                            g -> tile_color.r = 255;
                            g -> tile_color.g = 165;
                            g -> tile_color.b = 0;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_6:
                            g -> tile_color.r = 255;
                            g -> tile_color.g = 255;
                            g -> tile_color.b = 255;
                            g -> tile_color.a = 255;
                            running = false;
                            break;
                        case SDL_SCANCODE_C:
                            running = false;
                            struct Color chosen = open_color_slider_picker(g);
                            g -> tile_color = chosen;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        SDL_Delay(50); // Delay to reduce CPU usage
    }

    // Cleanup resources
    TTF_CloseFont(font);
    SDL_DestroyRenderer(cust_ren);
    SDL_DestroyWindow(cust_win);
    TTF_Quit();
}

/* --------------------------------------------------------------------------------------------
 * Text and Button Rendering
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Renders text onto the SDL renderer at a specified position.
 * 
 * This function creates an SDL surface from the given text using the provided font, converts
 * it into a texture, and then renders it at the desired coordinates.
 * 
 * @param ren The SDL_Renderer where the text will be drawn.
 * @param font The TTF_Font object to use when rendering the text.
 * @param text The null-terminated string to render.
 * @param x The X-coordinate of the top-left corner where the text will appear.
 * @param y The Y-coordinate of the top-left corner where the text will appear.
 * @param color The SDL_Color defining the color of the rendered text.
 * 
 * @note This function destroys the temporary SDL surface and texture after rendering.
 */

// Vanshi and Khushi
void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, text, strlen(text), color); // Create surface
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf); // Create texture from surface
    SDL_FRect dst = {x, y, surf->w, surf->h}; // Destination rectangle
    SDL_RenderTexture(ren, tex, NULL, &dst); // Render texture
    SDL_DestroyTexture(tex); // Clean up texture
    SDL_DestroySurface(surf); // Clean up surface
}

/**
 * @brief Draws a simple rectangular button with text.
 * 
 * This function renders a colored rectangle to represent a button and draws centered text inside
 * it using the @ref draw_text() function.
 * 
 * @param ren The SDL_Renderer used to draw the button.
 * @param font The TTF_Font object for rendering the button label.
 * @param rect The SDL_FRect defining the button's position and size.
 * @param text The label text to be displayed on the button.
 * @param color The SDL_Color specifying the text color.
 */

// Vanshi and Khushi
void draw_button(SDL_Renderer *ren, TTF_Font *font, SDL_FRect rect, const char *text, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, 0, 200, 0, 255); // Button background color
    SDL_RenderFillRect(ren, &rect); // Draw button rectangle
    draw_text(ren, font, text, rect.x + 10, rect.y + 10, color); // Draw button text
}

/**
 * @brief Displays a configuration window for customizing the placement of a preloaded pattern.
 * 
 * This function creates an interactive SDL_Window that allows the user to adjust the X and Y
 * offset positions for placing a pattern on the simulation grid and optionally choose whether
 * to clear the screen before loading it.
 * 
 * The user can:
 * - Increment/decrement the X and Y offset values using clickable "+" and "-" buttons.
 * - Toggle a checkbox to decide whether to clear the grid first.
 * - Confirm their settings using the "Apply" button.
 * 
 * Upon confirmation, the pattern specified by the given RLE file is loaded onto the grid using
 * the @ref load_rle() function with the selected options.
 * 
 * @param filename The file path to the RLE pattern to be loaded.
 * @param pattern_name The display name of the pattern, shown in the customization window title.
 * 
 * @note
 * This function creates and manages its own SDL_Window and SDL_renderer.
 * This function is blocking - it will not return until the user closes the customization window
 * or confirms the action.  
 */

// Vanshi and Khushi, Harmit and Yuvraj
void customize_preloaded_pattern(const char* filename, char pattern_name[]) {
    // Initialize pattern options with default values
    struct PatternOptions opts = {0, 0, false, false}; 

    // Initialize TTF for text rendering
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", SDL_GetError());
        return;
    }

    // Create customization window and renderer
    SDL_Window *win = SDL_CreateWindow("Conway's Game of Life | Pattern Options", 600, 400, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    // Load font for rendering text
    TTF_Font *font = TTF_OpenFont("assets/DejaVuSans.ttf", 20);
    if (!font) {
        SDL_Log("Error loading font: %s\n", SDL_GetError());
        return;
    }
    
    // Event loop for handling user input and rendering the customization interface
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (e.window.windowID == SDL_GetWindowID(win)) running = false;
            }
            // Handle mouse button clicks for adjusting options
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                // Increase X offset
                if (mx > 250 && mx < 270 && my > 80 && my < 100 && opts.offset_x < 29) {
                    opts.offset_x++;
                }
                // Decrease X offset
                if (mx > 200 && mx < 220 && my > 80 && my < 100 && opts.offset_x > 0) {
                    opts.offset_x--;
                }
                // Increase Y offset
                if (mx > 250 && mx < 270 && my > 120 && my < 140 && opts.offset_y < 26) {
                    opts.offset_y++;
                }
                // Decrease Y offset
                if (mx > 200 && mx < 220 && my > 120 && my < 140 && opts.offset_y > 0) {
                    opts.offset_y--;
                }
                // Toggle clear screen option
                if (mx > 50 && mx < 70 && my > 170 && my < 190) {
                    opts.clear = !opts.clear;
                }
                // Confirm and apply options
                if (mx > 80 && mx < 160 && my > 220 && my < 260) {
                    opts.confirmed = true;
                    running = false;
                }
            }
        }

        // Render the customization interface
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);

        SDL_Color white = {255, 255, 255, 255};

        // Draw option texts and interactive elements
        char buf[128];
        snprintf(buf, sizeof(buf), "Options for %s", pattern_name);
        draw_text(ren, font, buf, 20, 20, white);

        // Draw X offset option
        snprintf(buf, sizeof(buf), "X Offset: %d", opts.offset_x);
        draw_text(ren, font, buf, 80, 80, white);

        // Draw X offset adjustment buttons
        SDL_FRect plusX = {250, 80, 20, 20};
        SDL_FRect minusX = {200, 80, 20, 20};
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        SDL_RenderFillRect(ren, &plusX);
        SDL_RenderFillRect(ren, &minusX);
        draw_text(ren, font, "+", 252, 78, white);
        draw_text(ren, font, "-", 206, 78, white);

        // Draw Y offset option
        snprintf(buf, sizeof(buf), "Y Offset: %d", opts.offset_y);
        draw_text(ren, font, buf, 80, 120, white);

        // Draw Y offset adjustment buttons
        SDL_FRect plusY = {250, 120, 20, 20};
        SDL_FRect minusY = {200, 120, 20, 20};
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        SDL_RenderFillRect(ren, &plusY);
        SDL_RenderFillRect(ren, &minusY);
        draw_text(ren, font, "+", 252, 118, white);
        draw_text(ren, font, "-", 206, 118, white);

        // Draw clear screen checkbox
        SDL_FRect checkbox = {50, 170, 20, 20};
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderRect(ren, &checkbox);
        if (opts.clear) {
            SDL_FRect fillCheckbox = {50, 170, 20, 20};
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            SDL_RenderFillRect(ren, &fillCheckbox);
        }
        draw_text(ren, font, "Clear screen first", 80, 170, white);

        // Draw apply button
        SDL_FRect apply = {80, 220, 80, 40};
        draw_button(ren, font, apply, "Apply", white);

        SDL_RenderPresent(ren);
    }

    // Cleanup resources
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();

    // Load the pattern with the selected options if confirmed
    if (opts.confirmed) {
        printf("Loading pattern...\n");
        load_rle(filename, opts.offset_y, opts.offset_x, opts.clear);
    }

}

/* --------------------------------------------------------------------------------------------
 * Event Handling and Input
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Handles all SDL input events and updates the game state accordingly.
 * 
 * This function continuously polls SDL events (keyboard, mouse, and quit events) and performs
 * actions that modify the game's state, grid, and audio.
 * It also coordinates the background music and sound effects based on user input.
 * 
 * @param g Pointer to the active Game structure containing state information, renderer
 *          references, and control flags.
 * ### Event Controls:
 * - **ESC** - Exits the game.
 * - **SPACE** - Toggles between play and pause mode; pauses/resumes background music.
 * - **C** - Clears the grid, pauses the game, and plays a "clear" sound.
 * - **G** - Randomizes the grid pattern and plays a "randomization" sound.
 * - **N** - Advances the simulation by one generation when paused.
 * - **H** - Displays the help window with list of hotkeys.
 * - **P** - Opens the preloaded patterns menu.
 * - **S** - Opens the customization menu for tile colors.
 * - **1, 2, 3** - Loads  predefined patterns (Glider, Blinker, or Gospel Glider Gun).
 * - **UP / DOWN** - Adjusts the update frequency (simulation speed).
 * - **Mouse Click** - Toggles the state of the clicked cell and plays a toggle sound.
 * 
 * @note This function ensures responsive interaction by handling both keyboard and mouse inputs
 *       in the same loop. It also synchronizes audio playback with visual actions.
 */

// Het and Virat
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
                // Toggle cell state on mouse click
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

/* --------------------------------------------------------------------------------------------
 * Game Rendering and Main Loop
 * -------------------------------------------------------------------------------------------- 
 * This section manages all rendering and update logic for the game.
 * It handles drawing the grid, visualizing grid lines, refreshing frames on window,
 * and running the main game loop that updates the simulation state and processes
 * user interactions.
 * -------------------------------------------------------------------------------------------- */
 
/**
 * @brief Draws the grid lines on the game window.
 * 
 * This function renders a network of horizontal and vertical lines across the window to
 * visually separate individual cells.
 * It uses the defined TILE_SIZE to determine the spacing between lines.
 * 
 * @param g Pointer to the Game structure containing the renderer and state.
 * 
 * @note This function is purely visual and does not affect the grid state.
 */

// Vanshi and Khushi
void draw_grid_lines(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, 255, 255, 255, 255);
    // Draw vertical lines
    for (int x = 0; x < WINDOW_WIDTH; x+=TILE_SIZE) {
        SDL_RenderLine(g->renderer, x, 0, x, WINDOW_HEIGHT);
    }
    // Draw horizontal lines
    for (int y = 0; y < WINDOW_HEIGHT; y+=TILE_SIZE) {
        SDL_RenderLine(g->renderer, 0, y, WINDOW_WIDTH, y);
    }
}

/**
 * @brief Draws all active (alive) cells in the simulation grid.
 * 
 * Iterates through the `grid` array and fills each live cell as a colored rectangle using
 * colored rectangle using the current tile color from the Game struct.
 * 
 * @param g Pointer to the Game structure containing the renderer and color data.
 * 
 * @note This function only draws live cells (those with value `1`).
 *       Empty cells reamin black and drawn over by the background.
 */

// Vanshi and Khushi
void draw_grid(struct Game *g) {
    // Set draw color to the game's tile color
    SDL_SetRenderDrawColor(g->renderer, g->tile_color.r, g->tile_color.g, g->tile_color.b, g->tile_color.a);
    // Iterate through the grid and draw live cells
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x]) {
                // Draw filled rectangle for live cell
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

/**
 * @brief Draws a single frame of the game window.
 * 
 * Clears the renderer, redraws all live cells and grid lines, and presents the final
 * composed frame to the display.
 * 
 * @param g Pointer to the game instance.
 */

// Vanshi and Khushi
void game_draw(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(g->renderer);
    draw_grid(g);
    draw_grid_lines(g);
    SDL_RenderPresent(g->renderer);
}

/**
 * @brief Main simulation loop for the game.
 * 
 * Continuously updates the game state, processes events, and redraws the frame as long as
 * the game is running.
 * The update frequency controls how often the grid evolves to the next generation.
 * 
 * @param g Pointer to the active Game structure containing window, renderer, and state
 *          information.
 * 
 * @note The window title dynamically updates to reflect the current play or pause status.
 */

// Vanshi and Khushi, Prateek and Hunar
void game_run(struct Game *g) {
    int count = 0; // Frame counter for update frequency

    while (g -> is_running) {
        if (g -> is_playing) {
            count++; // Increment frame counter
        }
        // Update grid if enough frames have passed
        if (count >= g -> update_freq) {
            count = 0;
            update_grid();
        }
        // Update window title based on play/pause state
        SDL_SetWindowTitle(g->window, g->is_playing? "Conway's Game of Life | Playing" : "Conway's Game of Life | Paused");
        
        // Handle events and draw the frame
        game_events(g);
        game_draw(g);

        SDL_Delay(16); // Approx ~60 FPS
    }
}

/* --------------------------------------------------------------------------------------------
 * Program Entry Point
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Entry point of the Conway's Game of Life simulation.
 * 
 * Initializes the game environment, starts the main simulation loop, and ensures proper
 * cleanup after the program terminates.
 * 
 * The function first attempts to initialize all game components using `game_new()`.
 * If successful, it enters the main loop via `game_run()`, which handles event processing,
 * simulation updates, and rendering. Upon exit, all allocated resources are released using
 * `game_free()`.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * 
 * @return EXIT_SUCCESS if the game ran successfully, otherwise EXIT_FAILURE.
 */

int main(int argc, char *argv[]) {
    bool exit_status = EXIT_FAILURE; // Default to failure

    struct Game game = {0};

    // Initialize game components
    if (game_new(&game)) {
        game_run(&game);
        exit_status = EXIT_SUCCESS; // Set success if run completes
    }

    // Clean up allocated resources
    game_free(&game);

    return exit_status;
}