#ifndef ARENA_INPUT_H
#define ARENA_INPUT_H

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Key Codes (matches GLFW)
// ============================================================================

typedef enum Key {
    KEY_UNKNOWN = -1,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_0 = 48, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT = 96,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_MAX = 512
} Key;

// ============================================================================
// Mouse Buttons
// ============================================================================

typedef enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_4 = 3,
    MOUSE_BUTTON_5 = 4,
    MOUSE_BUTTON_MAX = 8
} MouseButton;

// ============================================================================
// Input State
// ============================================================================

typedef struct InputState {
    // Keyboard
    bool keys[KEY_MAX];
    bool keys_previous[KEY_MAX];
    
    // Mouse
    bool mouse_buttons[MOUSE_BUTTON_MAX];
    bool mouse_buttons_previous[MOUSE_BUTTON_MAX];
    double mouse_x, mouse_y;
    double mouse_dx, mouse_dy;        // Delta since last frame
    double scroll_x, scroll_y;        // Scroll wheel
    
    // Modifiers
    bool shift_down;
    bool ctrl_down;
    bool alt_down;
    bool super_down;
} InputState;

// ============================================================================
// Input API
// ============================================================================

// Initialize/shutdown
void input_init(InputState* state);

// Call at start of frame to update previous state
void input_update(InputState* state);

// Key state queries
bool input_key_down(const InputState* state, Key key);
bool input_key_pressed(const InputState* state, Key key);   // Just pressed this frame
bool input_key_released(const InputState* state, Key key);  // Just released this frame

// Mouse state queries
bool input_mouse_down(const InputState* state, MouseButton button);
bool input_mouse_pressed(const InputState* state, MouseButton button);
bool input_mouse_released(const InputState* state, MouseButton button);
void input_mouse_position(const InputState* state, double* x, double* y);
void input_mouse_delta(const InputState* state, double* dx, double* dy);

// Movement helpers (returns normalized direction from WASD or arrows)
void input_get_movement(const InputState* state, float* out_x, float* out_y);

// GLFW callbacks (set these on window)
struct GLFWwindow;
void input_glfw_key_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods);
void input_glfw_mouse_button_callback(struct GLFWwindow* window, int button, int action, int mods);
void input_glfw_cursor_pos_callback(struct GLFWwindow* window, double x, double y);
void input_glfw_scroll_callback(struct GLFWwindow* window, double x, double y);

// Set the global input state that callbacks will update
void input_set_callback_target(InputState* state);

#endif

