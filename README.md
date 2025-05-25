Header-Only SDL2 Virtual Joystick
A lightweight, header-only virtual joystick library for SDL2, designed for touchscreens and compatible with both C and C++ projects. This joystick is optimized for simplicity and ease of integration, providing essential functionality without unnecessary bloat.
Features:
 * Header-Only: Simply include virtual_joystick.h in your project – no separate .c or .cpp files to compile.
 * Lightweight: Minimalistic codebase, ensuring a small footprint (approx. 25KB).
 * C/C++ Compatible: Designed with extern "C" to work seamlessly in both C and C++ environments.
 * Semi-Transparent & Grayscale: Modern, subtle visual design.
 * Dynamic Visibility: Appears only when the screen is touched within its interaction area and disappears when released.
 * Left 1/3 Screen Placement: Configured to operate within the left third of the screen, ideal for mobile game controls.
 * Responsive: Adapts to window resizing.
Usage:
To use this joystick in your project, simply include virtual_joystick.h.
Compiling as a Standalone Demo:
You can compile the header file directly to run the included demo application:
# For C
gcc virtual_joystick.h -o joystick_demo -lSDL2 -lm

# For C++
g++ virtual_joystick.h -o joystick_demo -lSDL2 -lm

Integrating into Your Project:
If you have your own main function, define VIRTUAL_JOYSTICK_NO_MAIN before including the header to prevent its main function from being compiled:
```
#define VIRTUAL_JOYSTICK_IMPLEMENTATION // Required to include function definitions
#define VIRTUAL_JOYSTICK_NO_MAIN        // Prevents the demo's main function from being included
#include "virtual_joystick.h"

int main(int argc, char* args[]) {
    // Your SDL initialization and game loop
    SDL_Renderer* renderer = /* ... */;
    int win_width = 800, win_height = 600; // Example dimensions

    // Define joystick area (e.g., left 1/3 of screen)
    int joystick_area_width = win_width / 3;
    int joystick_area_height = win_height;
    int joystick_area_x = 0;
    int joystick_area_y = 0;

    VirtualJoystick* myJoystick = VirtualJoystick_Create(renderer, joystick_area_x, joystick_area_y, joystick_area_width, joystick_area_height, win_width, win_height);

    // In your event loop:
    // VirtualJoystick_HandleEvent(myJoystick, &e);

    // In your rendering loop:
    // VirtualJoystick_Draw(myJoystick, renderer);

    // On window resize:
    // VirtualJoystick_SetWindowSize(myJoystick, new_width, new_height);

    // When done:
    VirtualJoystick_Destroy(myJoystick);

    return 0;
}


License:
This project is licensed under the MIT License. See the LICENSE.md file for details.
Developed by Ferki (denisdola278@gmail.com)
