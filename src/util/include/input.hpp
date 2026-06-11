#pragma once

#include <SDL3/SDL_events.h>
#include <cstdint>

enum class MovementDirection : uint8_t { none = 0, forward = 1 << 0, backward = 1 << 1, left = 1 << 2, right = 1 << 3 };

inline auto operator|(MovementDirection a, MovementDirection b) -> MovementDirection {
    return static_cast<MovementDirection>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline auto operator|=(MovementDirection& a, MovementDirection b) -> MovementDirection& {
    a = a | b;
    return a;
}

inline auto operator&(MovementDirection a, MovementDirection b) -> MovementDirection {
    return static_cast<MovementDirection>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline auto has(MovementDirection value, MovementDirection flag) -> bool {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

struct Input {
    MovementDirection movement = MovementDirection::none;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseScrollY = 0.0f;

    // Could be lots of mouse movements per frame, we only care about the final result
    void updateMouse(SDL_MouseMotionEvent motion) {
        mouseX += motion.xrel;
        mouseY += motion.yrel;
    };

    void updateScroll(SDL_MouseWheelEvent wheel) { mouseScrollY += wheel.y; }

    void updateKeys(const bool* keys) {
        movement = MovementDirection::none;

        if (keys[SDL_SCANCODE_W]) {
            movement |= MovementDirection::forward;
        }
        if (keys[SDL_SCANCODE_S]) {
            movement |= MovementDirection::backward;
        }
        if (keys[SDL_SCANCODE_A]) {
            movement |= MovementDirection::left;
        }
        if (keys[SDL_SCANCODE_D]) {
            movement |= MovementDirection::right;
        }
    }
};