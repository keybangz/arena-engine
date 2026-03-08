#include "input.h"
#include <string.h>
#include <math.h>

// Global input state for GLFW callbacks
static InputState* g_input_state = NULL;

// ============================================================================
// Initialization
// ============================================================================

void input_init(InputState* state) {
    memset(state, 0, sizeof(InputState));
}

void input_update(InputState* state) {
    // Copy current state to previous
    memcpy(state->keys_previous, state->keys, sizeof(state->keys));
    memcpy(state->mouse_buttons_previous, state->mouse_buttons, sizeof(state->mouse_buttons));
    
    // Reset deltas
    state->mouse_dx = 0.0;
    state->mouse_dy = 0.0;
    state->scroll_x = 0.0;
    state->scroll_y = 0.0;
}

// ============================================================================
// Key State Queries
// ============================================================================

bool input_key_down(const InputState* state, Key key) {
    if (key < 0 || key >= KEY_MAX) return false;
    return state->keys[key];
}

bool input_key_pressed(const InputState* state, Key key) {
    if (key < 0 || key >= KEY_MAX) return false;
    return state->keys[key] && !state->keys_previous[key];
}

bool input_key_released(const InputState* state, Key key) {
    if (key < 0 || key >= KEY_MAX) return false;
    return !state->keys[key] && state->keys_previous[key];
}

// ============================================================================
// Mouse State Queries
// ============================================================================

bool input_mouse_down(const InputState* state, MouseButton button) {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return false;
    return state->mouse_buttons[button];
}

bool input_mouse_pressed(const InputState* state, MouseButton button) {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return false;
    return state->mouse_buttons[button] && !state->mouse_buttons_previous[button];
}

bool input_mouse_released(const InputState* state, MouseButton button) {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return false;
    return !state->mouse_buttons[button] && state->mouse_buttons_previous[button];
}

void input_mouse_position(const InputState* state, double* x, double* y) {
    if (x) *x = state->mouse_x;
    if (y) *y = state->mouse_y;
}

void input_mouse_delta(const InputState* state, double* dx, double* dy) {
    if (dx) *dx = state->mouse_dx;
    if (dy) *dy = state->mouse_dy;
}

// ============================================================================
// Movement Helper
// ============================================================================

void input_get_movement(const InputState* state, float* out_x, float* out_y) {
    float x = 0.0f, y = 0.0f;
    
    // WASD
    if (state->keys[KEY_W] || state->keys[KEY_UP])    y += 1.0f;
    if (state->keys[KEY_S] || state->keys[KEY_DOWN])  y -= 1.0f;
    if (state->keys[KEY_A] || state->keys[KEY_LEFT])  x -= 1.0f;
    if (state->keys[KEY_D] || state->keys[KEY_RIGHT]) x += 1.0f;
    
    // Normalize diagonal movement
    float len = sqrtf(x * x + y * y);
    if (len > 0.0f) {
        x /= len;
        y /= len;
    }
    
    if (out_x) *out_x = x;
    if (out_y) *out_y = y;
}

// ============================================================================
// GLFW Callbacks
// ============================================================================

void input_set_callback_target(InputState* state) {
    g_input_state = state;
}

void input_glfw_key_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode;
    
    if (!g_input_state) return;
    if (key < 0 || key >= KEY_MAX) return;
    
    if (action == 1) {  // GLFW_PRESS
        g_input_state->keys[key] = true;
    } else if (action == 0) {  // GLFW_RELEASE
        g_input_state->keys[key] = false;
    }
    
    // Update modifiers
    g_input_state->shift_down = (mods & 0x0001) != 0;  // GLFW_MOD_SHIFT
    g_input_state->ctrl_down  = (mods & 0x0002) != 0;  // GLFW_MOD_CONTROL
    g_input_state->alt_down   = (mods & 0x0004) != 0;  // GLFW_MOD_ALT
    g_input_state->super_down = (mods & 0x0008) != 0;  // GLFW_MOD_SUPER
}

void input_glfw_mouse_button_callback(struct GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    
    if (!g_input_state) return;
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return;
    
    g_input_state->mouse_buttons[button] = (action == 1);  // GLFW_PRESS
}

void input_glfw_cursor_pos_callback(struct GLFWwindow* window, double x, double y) {
    (void)window;
    
    if (!g_input_state) return;
    
    g_input_state->mouse_dx = x - g_input_state->mouse_x;
    g_input_state->mouse_dy = y - g_input_state->mouse_y;
    g_input_state->mouse_x = x;
    g_input_state->mouse_y = y;
}

void input_glfw_scroll_callback(struct GLFWwindow* window, double x, double y) {
    (void)window;
    
    if (!g_input_state) return;
    
    g_input_state->scroll_x = x;
    g_input_state->scroll_y = y;
}

