#ifndef VIRTUAL_JOYSTICK_H
#define VIRTUAL_JOYSTICK_H

#include <SDL2/SDL.h> // If you are viewing this code in a public view (for example, in GitHub), then I advise you to change this path to your real path so that the code works correctly.
#include <stdbool.h>
#include <math.h>
#include <stdio.h>  // For fprintf, printf
#include <stdlib.h> // For malloc, free

// This block ensures that the C functions are compatible with C++ compilers.
// It prevents name mangling, allowing C++ code to link with C functions.
#ifdef __cplusplus
extern "C" {
#endif

// --- Vector2 Structure and Operations ---
// A simple 2D vector structure to represent joystick output and positions.
typedef struct {
    float x;
    float y;
} Vector2;

// Calculates the Euclidean length (magnitude) of a Vector2.
// Marked as static inline to allow the compiler to potentially optimize by inlining the function
// and to prevent multiple definition errors when included in multiple translation units.
static inline float Vector2_Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

// Normalizes a Vector2 to a unit vector (length of 1). Returns (0,0) if input is zero vector.
static inline Vector2 Vector2_Normalize(Vector2 v) {
    float len = Vector2_Length(v);
    if (len == 0.0f) return (Vector2){0.0f, 0.0f};
    return (Vector2){v.x / len, v.y / len};
}

// Limits the length of a Vector2 to a specified maximum length.
static inline Vector2 Vector2_LimitLength(Vector2 v, float max_len) {
    float len = Vector2_Length(v);
    if (len > max_len) {
        return (Vector2){v.x / len * max_len, v.y / len * max_len};
    }
    return v;
}

// --- Joystick Mode Enumeration ---
// Defines how the joystick behaves when touched.
typedef enum {
    JOYSTICK_MODE_FIXED,    // The joystick stays in its initial position.
    JOYSTICK_MODE_DYNAMIC,  // The joystick appears at the touched position.
    JOYSTICK_MODE_FOLLOWING // The joystick follows the finger if it moves outside the clampzone.
} JoystickMode;

// --- VirtualJoystick Structure ---
// Represents the state and properties of the virtual joystick.
typedef struct {
    SDL_Renderer* renderer;        // SDL renderer used for drawing.
    SDL_Rect joystick_area;        // The overall rectangular area on screen where the joystick operates.

    SDL_Color pressed_color;       // Color of the tip when the joystick is pressed.
    float deadzone_size;           // Input inside this range yields zero output.
    float clampzone_size;          // Maximum distance the tip can move from the base center.
    JoystickMode joystick_mode;    // Current mode of the joystick (FIXED, DYNAMIC, FOLLOWING).

    bool is_pressed;               // True if the joystick is currently being pressed.
    Vector2 output;                // The normalized output vector (from -1 to 1 in X and Y).

    int _touch_index;              // The ID of the finger currently interacting with the joystick (-1 if none).

    SDL_Texture* _base_texture;    // Texture for the joystick's base.
    SDL_Texture* _tip_texture;     // Texture for the joystick's movable tip.

    SDL_FPoint _base_center;       // Current center position of the base on screen.
    SDL_FPoint _tip_center;        // Current center position of the tip on screen.

    SDL_FPoint _base_default_center; // Initial (default) center position of the base.

    SDL_Color _default_tip_color;  // The default color of the tip texture.
    int _base_radius;              // Radius of the base circle.
    int _tip_radius;               // Radius of the tip circle.

    bool _hidden;                  // If true, the joystick is not drawn or processed.
    int _window_width;             // Stored window width for touch coordinate conversion.
    int _window_height;            // Stored window height for touch coordinate conversion.
} VirtualJoystick;

// --- Function Prototypes (Public API) ---
// These functions are the main interface for interacting with the VirtualJoystick.
VirtualJoystick* VirtualJoystick_Create(SDL_Renderer* renderer, int x, int y, int width, int height, int window_width, int window_height);
void VirtualJoystick_Destroy(VirtualJoystick* joystick);
void VirtualJoystick_SetWindowSize(VirtualJoystick* joystick, int width, int height);
void VirtualJoystick_HandleEvent(VirtualJoystick* joystick, const SDL_Event* event);
void VirtualJoystick_Draw(VirtualJoystick* joystick, SDL_Renderer* renderer);


// --- Implementation Block ---
// This block contains the actual definitions of the functions.
// It is guarded by VIRTUAL_JOYSTICK_IMPLEMENTATION.
// If you include this header in a .c/.cpp file and want to compile the main
// application, you can define VIRTUAL_JOYSTICK_IMPLEMENTATION *before* including
// this header. If you just compile this .h file directly, it will define it itself.
#if !defined(VIRTUAL_JOYSTICK_IMPLEMENTATION) && !defined(VIRTUAL_JOYSTICK_NO_MAIN)
    #define VIRTUAL_JOYSTICK_IMPLEMENTATION // Automatically define if not explicitly prevented
#endif

#ifdef VIRTUAL_JOYSTICK_IMPLEMENTATION

// --- Helper Function: create_circle_texture ---
// Creates an SDL_Texture containing a filled circle. This is used to draw the joystick's base and tip.
// Parameters:
//   renderer: The SDL_Renderer to create the texture with.
//   radius: The radius of the circle to draw.
//   color: The SDL_Color of the circle.
// Returns: An SDL_Texture* on success, NULL on failure.
static inline SDL_Texture* create_circle_texture(SDL_Renderer* renderer, int radius, SDL_Color color) {
    int diameter = radius * 2;
    // Create a texture with RGBA format and render target access.
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, diameter, diameter);
    if (!texture) {
        fprintf(stderr, "Failed to create circle texture: %s\n", SDL_GetError());
        return NULL;
    }

    // Enable blending for transparency.
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    // Set the texture as the current rendering target.
    SDL_SetRenderTarget(renderer, texture);

    // Clear the texture with transparent color before drawing.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Transparent black
    SDL_RenderClear(renderer);

    // Draw filled circle by iterating pixels within the radius.
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                SDL_RenderDrawPoint(renderer, radius + x, radius + y);
            }
        }
    }

    // Reset render target to the default window renderer.
    SDL_SetRenderTarget(renderer, NULL);
    return texture;
}

// --- Helper Function: _is_point_inside_joystick_area ---
// Checks if a given point is within the overall interaction area of the joystick.
static inline bool _is_point_inside_joystick_area(const VirtualJoystick* joystick, SDL_FPoint point) {
    SDL_Point p = {(int)point.x, (int)point.y};
    return SDL_PointInRect(&p, &joystick->joystick_area);
}

// --- Helper Function: _is_point_inside_base ---
// Checks if a given point is within the circular base of the joystick.
static inline bool _is_point_inside_base(const VirtualJoystick* joystick, SDL_FPoint point) {
    float dx = point.x - joystick->_base_center.x;
    float dy = point.y - joystick->_base_center.y;
    return (dx * dx + dy * dy) <= (joystick->_base_radius * joystick->_base_radius);
}

// --- Helper Function: _move_base ---
// Updates the center position of the joystick's base.
static inline void _move_base(VirtualJoystick* joystick, SDL_FPoint new_center) {
    joystick->_base_center = new_center;
}

// --- Helper Function: _move_tip ---
// Updates the center position of the joystick's tip.
static inline void _move_tip(VirtualJoystick* joystick, SDL_FPoint new_center) {
    joystick->_tip_center = new_center;
}

// --- Helper Function: _update_joystick_logic ---
// Contains the core logic for calculating joystick output and updating tip position
// based on the current touch position.
static inline void _update_joystick_logic(VirtualJoystick* joystick, SDL_FPoint touch_position) {
    // Calculate vector from base center to touch position.
    Vector2 vector_from_base_center = (Vector2){touch_position.x - joystick->_base_center.x,
                                                touch_position.y - joystick->_base_center.y};

    // Limit the vector length to the clampzone_size.
    Vector2 clamped_vector = Vector2_LimitLength(vector_from_base_center, joystick->clampzone_size);

    // Handle FOLLOWING joystick mode: move the base if finger moves outside clampzone.
    if (joystick->joystick_mode == JOYSTICK_MODE_FOLLOWING && Vector2_Length(vector_from_base_center) > joystick->clampzone_size) {
        // Calculate new base center to keep the tip at the clampzone edge.
        SDL_FPoint new_base_center = (SDL_FPoint){touch_position.x - clamped_vector.x,
                                                  touch_position.y - clamped_vector.y};
        _move_base(joystick, new_base_center);
    }

    // Update tip position relative to the current base center.
    _move_tip(joystick, (SDL_FPoint){joystick->_base_center.x + clamped_vector.x,
                                     joystick->_base_center.y + clamped_vector.y});

    // Calculate joystick output based on deadzone and clampzone.
    if (Vector2_Length(clamped_vector) > joystick->deadzone_size) {
        joystick->is_pressed = true;
        Vector2 normalized_vector = Vector2_Normalize(clamped_vector);
        float effective_length = Vector2_Length(clamped_vector) - joystick->deadzone_size;
        float max_effective_length = joystick->clampzone_size - joystick->deadzone_size;

        if (max_effective_length <= 0.0f) { // Prevent division by zero if clampzone <= deadzone.
            joystick->output = (Vector2){0.0f, 0.0f};
        } else {
            // Normalize output to range -1 to 1.
            joystick->output = (Vector2){normalized_vector.x * (effective_length / max_effective_length),
                                         normalized_vector.y * (effective_length / max_effective_length)};
        }
    } else {
        joystick->is_pressed = false;
        joystick->output = (Vector2){0.0f, 0.0f};
    }
}

// --- Helper Function: _reset_joystick ---
// Resets the joystick to its default, unpressed state.
static inline void _reset_joystick(VirtualJoystick* joystick) {
    joystick->is_pressed = false;
    joystick->output = (Vector2){0.0f, 0.0f};
    joystick->_touch_index = -1; // No finger is currently touching.

    // Reset tip color to default.
    SDL_SetTextureColorMod(joystick->_tip_texture, joystick->_default_tip_color.r, joystick->_default_tip_color.g, joystick->_default_tip_color.b);

    // Reset base and tip positions to their defaults.
    _move_base(joystick, joystick->_base_default_center);
    _move_tip(joystick, joystick->_base_default_center); // Tip goes back to base center.

    joystick->_hidden = true; // Hide the joystick after release.
}


// --- VirtualJoystick_Create ---
// Initializes and allocates a new VirtualJoystick instance.
// Parameters:
//   renderer: The SDL_Renderer to be used for drawing the joystick.
//   x, y: The top-left coordinates of the joystick's overall interaction area.
//   width, height: The dimensions of the joystick's overall interaction area.
//   window_width, window_height: The current dimensions of the window.
// Returns: A pointer to the newly created VirtualJoystick on success, NULL on failure.
VirtualJoystick* VirtualJoystick_Create(SDL_Renderer* renderer, int x, int y, int width, int height, int window_width, int window_height) {
    VirtualJoystick* joystick = (VirtualJoystick*)malloc(sizeof(VirtualJoystick));
    if (!joystick) {
        fprintf(stderr, "Failed to allocate VirtualJoystick\n");
        return NULL;
    }

    joystick->renderer = renderer;
    joystick->joystick_area = (SDL_Rect){x, y, width, height};
    joystick->_window_width = window_width;
    joystick->_window_height = window_height;

    // Set default properties.
    joystick->pressed_color = (SDL_Color){100, 100, 100, 180}; // Medium gray, semi-transparent
    joystick->deadzone_size = 10.0f;
    joystick->clampzone_size = 75.0f;
    joystick->joystick_mode = JOYSTICK_MODE_DYNAMIC; // Dynamic mode by default

    joystick->is_pressed = false;
    joystick->output = (Vector2){0.0f, 0.0f};
    joystick->_touch_index = -1;

    // Calculate radii for base and tip based on the joystick area size.
    joystick->_base_radius = (int)(fmin(width, height) * 0.25f);
    joystick->_tip_radius = (int)(joystick->_base_radius * 0.6f);

    // Create textures for the base and tip.
    joystick->_default_tip_color = (SDL_Color){200, 200, 200, 180}; // Light gray, semi-transparent
    joystick->_base_texture = create_circle_texture(renderer, joystick->_base_radius, (SDL_Color){50, 50, 50, 180}); // Dark gray, semi-transparent
    joystick->_tip_texture = create_circle_texture(renderer, joystick->_tip_radius, joystick->_default_tip_color);

    if (!joystick->_base_texture || !joystick->_tip_texture) {
        VirtualJoystick_Destroy(joystick); // Clean up if texture creation fails.
        return NULL;
    }

    // Set initial default positions for the base and tip.
    joystick->_base_default_center = (SDL_FPoint){joystick->joystick_area.x + joystick->joystick_area.w / 2.0f,
                                                  joystick->joystick_area.y + joystick->joystick_area.h / 2.0f};
    joystick->_base_center = joystick->_base_default_center;
    joystick->_tip_center = joystick->_base_center; // Initially, tip is at base center.

    joystick->_hidden = true; // Joystick is hidden by default.

    return joystick;
}

// --- VirtualJoystick_Destroy ---
// Frees all resources associated with the VirtualJoystick.
// Parameters:
//   joystick: A pointer to the VirtualJoystick instance to destroy.
void VirtualJoystick_Destroy(VirtualJoystick* joystick) {
    if (joystick) {
        if (joystick->_base_texture) {
            SDL_DestroyTexture(joystick->_base_texture);
        }
        if (joystick->_tip_texture) {
            SDL_DestroyTexture(joystick->_tip_texture);
        }
        free(joystick);
    }
}

// --- VirtualJoystick_SetWindowSize ---
// Updates the stored window dimensions within the joystick.
// Parameters:
//   joystick: A pointer to the VirtualJoystick instance.
//   width: The new width of the window.
//   height: The new height of the window.
void VirtualJoystick_SetWindowSize(VirtualJoystick* joystick, int width, int height) {
    if (joystick) {
        joystick->_window_width = width;
        joystick->_window_height = height;
    }
}

// --- VirtualJoystick_HandleEvent ---
// Processes SDL events relevant to the joystick (touch input).
// Parameters:
//   joystick: A pointer to the VirtualJoystick instance.
//   event: A pointer to the SDL_Event to process.
void VirtualJoystick_HandleEvent(VirtualJoystick* joystick, const SDL_Event* event) {
    // If the joystick is hidden, only process SDL_FINGERDOWN to make it appear.
    if (joystick->_hidden && event->type != SDL_FINGERDOWN) return;

    switch (event->type) {
        case SDL_FINGERDOWN: {
            // Convert normalized touch coordinates (0.0 to 1.0) to screen pixel coordinates.
            SDL_FPoint touch_pos = {(float)event->tfinger.x * joystick->_window_width,
                                    (float)event->tfinger.y * joystick->_window_height};

            // Check if the touch is within the joystick's interaction area and no other finger is tracking it.
            if (_is_point_inside_joystick_area(joystick, touch_pos) && joystick->_touch_index == -1) {
                bool should_activate = false;
                if (joystick->joystick_mode == JOYSTICK_MODE_DYNAMIC || joystick->joystick_mode == JOYSTICK_MODE_FOLLOWING) {
                    should_activate = true;
                } else if (joystick->joystick_mode == JOYSTICK_MODE_FIXED && _is_point_inside_base(joystick, touch_pos)) {
                    should_activate = true;
                }

                if (should_activate) {
                    // In DYNAMIC/FOLLOWING mode, move the base to the touch position.
                    if (joystick->joystick_mode == JOYSTICK_MODE_DYNAMIC || joystick->joystick_mode == JOYSTICK_MODE_FOLLOWING) {
                        _move_base(joystick, touch_pos);
                    }
                    joystick->_touch_index = event->tfinger.fingerId; // Start tracking this finger.
                    joystick->_hidden = false; // Make the joystick visible.
                    // Change tip color to indicate pressed state.
                    SDL_SetTextureColorMod(joystick->_tip_texture, joystick->pressed_color.r, joystick->pressed_color.g, joystick->pressed_color.b);
                    _update_joystick_logic(joystick, touch_pos); // Update joystick state immediately.
                }
            }
            break;
        }
        case SDL_FINGERUP: {
            // If the released finger is the one tracking the joystick, reset the joystick.
            if (event->tfinger.fingerId == joystick->_touch_index) {
                _reset_joystick(joystick);
            }
            break;
        }
        case SDL_FINGERMOTION: {
            // If the moving finger is the one tracking the joystick, update its state.
            if (event->tfinger.fingerId == joystick->_touch_index) {
                SDL_FPoint touch_pos = {(float)event->tfinger.x * joystick->_window_width,
                                        (float)event->tfinger.y * joystick->_window_height};
                _update_joystick_logic(joystick, touch_pos);
            }
            break;
        }
    }
}

// --- VirtualJoystick_Draw ---
// Renders the joystick onto the screen.
// Parameters:
//   joystick: A pointer to the VirtualJoystick instance.
//   renderer: The SDL_Renderer to draw with.
void VirtualJoystick_Draw(VirtualJoystick* joystick, SDL_Renderer* renderer) {
    if (joystick->_hidden) return; // Don't draw if hidden.

    // Calculate destination rectangle for the base texture.
    SDL_Rect base_dst_rect = {
        (int)(joystick->_base_center.x - joystick->_base_radius),
        (int)(joystick->_base_center.y - joystick->_base_radius),
        joystick->_base_radius * 2,
        joystick->_base_radius * 2
    };
    // Render the base texture.
    SDL_RenderCopy(renderer, joystick->_base_texture, NULL, &base_dst_rect);

    // Calculate destination rectangle for the tip texture.
    SDL_Rect tip_dst_rect = {
        (int)(joystick->_tip_center.x - joystick->_tip_radius),
        (int)(joystick->_tip_center.y - joystick->_tip_radius),
        joystick->_tip_radius * 2,
        joystick->_tip_radius * 2
    };
    // Render the tip texture.
    SDL_RenderCopy(renderer, joystick->_tip_texture, NULL, &tip_dst_rect);
}

// --- Main Application Entry Point ---
// This main function is included only if VIRTUAL_JOYSTICK_IMPLEMENTATION is defined.
// This allows the header to be compiled directly as an executable.
#ifndef VIRTUAL_JOYSTICK_NO_MAIN // Allows users to exclude main if they define their own
int main(int argc, char* args[]) {
    // Initialize SDL subsystems (Video and Events are needed for graphics and input).
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create an SDL window.
    SDL_Window* window = SDL_CreateWindow("Virtual Joystick SDL2 Demo",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          800, 600,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create an SDL renderer for drawing.
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Get current window dimensions to define the joystick's interaction area.
    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);

    // Define the joystick's interaction area: left 1/3 of the screen.
    int joystick_area_width = win_width / 3;
    int joystick_area_height = win_height;
    int joystick_area_x = 0;
    int joystick_area_y = 0;

    // Create the Virtual Joystick instance, passing window dimensions.
    VirtualJoystick* joystick = VirtualJoystick_Create(renderer, joystick_area_x, joystick_area_y, joystick_area_width, joystick_area_height, win_width, win_height);
    if (!joystick) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Customize joystick properties for demonstration.
    joystick->joystick_mode = JOYSTICK_MODE_DYNAMIC; // Dynamic mode for appearance on touch.
    joystick->clampzone_size = 100.0f;
    joystick->deadzone_size = 20.0f;

    bool quit = false; // Flag to control the main game loop.
    SDL_Event e;       // SDL event structure to hold incoming events.

    // --- Main Game Loop ---
    while (!quit) {
        // Event processing loop.
        while (SDL_PollEvent(&e) != 0) {
            // Check for quit event (e.g., closing the window).
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            // Handle window resize events to make the joystick responsive.
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_GetWindowSize(window, &win_width, &win_height);
                VirtualJoystick_SetWindowSize(joystick, win_width, win_height); // Update window size in joystick.

                // Update the joystick's interaction area to the new left 1/3.
                joystick->joystick_area = (SDL_Rect){0, 0, win_width / 3, win_height};
                // Recalculate and reset the base's default center based on the new area.
                joystick->_base_default_center = (SDL_FPoint){joystick->joystick_area.x + joystick->joystick_area.w / 2.0f,
                                                              joystick->joystick_area.y + joystick->joystick_area.h / 2.0f};
                _reset_joystick(joystick); // Reset joystick and hide it after resize.
            }
            // Pass all events to the joystick for its internal handling.
            VirtualJoystick_HandleEvent(joystick, &e);
        }

        // --- Rendering ---
        // Set the drawing color to black and clear the renderer (background).
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        // Draw the virtual joystick.
        VirtualJoystick_Draw(joystick, renderer);

        // Display joystick output (for debugging/demonstration purposes).
        printf("\rOutput: X=%.2f, Y=%.2f (Pressed: %s)       ",
               joystick->output.x, joystick->output.y, joystick->is_pressed ? "Yes" : "No");
        fflush(stdout); // Ensure the console output is updated.

        // Present the rendered frame to the window.
        SDL_RenderPresent(renderer);
    }

    // --- Cleanup ---
    // Destroy the VirtualJoystick instance.
    VirtualJoystick_Destroy(joystick);
    // Destroy the renderer.
    SDL_DestroyRenderer(renderer);
    // Destroy the window.
    SDL_DestroyWindow(window);
    // Quit SDL subsystems.
    SDL_Quit();

    return 0;
}
#endif // VIRTUAL_JOYSTICK_NO_MAIN

#endif // VIRTUAL_JOYSTICK_IMPLEMENTATION

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VIRTUAL_JOYSTICK_H

