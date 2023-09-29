#pragma once

#include <Windows.h>
#include <queue>
#include <cstdint>

class Input
{
public:
    struct MouseState
    {
        float delta_x = 0.f;
        float delta_y = 0.f;
        float delta_z = 0.f; // scroll

        struct MouseButtonState {
            bool pressed = false;
            bool released = false;
        };
        MouseButtonState lbutton;
        MouseButtonState mbutton;
        MouseButtonState rbutton;
    };

#define FOR_EACH_BUTTON(FUNC)   \
    FUNC(q, 'Q');               \
    FUNC(w, 'W');               \
    FUNC(e, 'E');               \
    FUNC(r, 'R');               \
    FUNC(t, 'T');               \
    FUNC(y, 'Y');               \
    FUNC(u, 'U');               \
    FUNC(i, 'I');               \
    FUNC(o, 'O');               \
    FUNC(p, 'P');               \
    FUNC(a, 'A');               \
    FUNC(s, 'S');               \
    FUNC(d, 'D');               \
    FUNC(f, 'F');               \
    FUNC(g, 'G');               \
    FUNC(h, 'H');               \
    FUNC(j, 'J');               \
    FUNC(k, 'K');               \
    FUNC(l, 'L');               \
    FUNC(z, 'Z');               \
    FUNC(x, 'X');               \
    FUNC(c, 'C');               \
    FUNC(v, 'V');               \
    FUNC(b, 'B');               \
    FUNC(n, 'N');               \
    FUNC(m, 'M');               \
    FUNC(shift, VK_SHIFT);      \
    FUNC(ctrl,  VK_CONTROL);    \
    FUNC(alt,   VK_MENU);       \
    FUNC(space, VK_SPACE);      \
    FUNC(enter, VK_RETURN);     \
    FUNC(up,    VK_UP);         \
    FUNC(down,  VK_DOWN);       \
    FUNC(left,  VK_LEFT);       \
    FUNC(right, VK_RIGHT);

    struct KeyboardState {
        struct ButtonState
        {
            bool pressed{ false };
            bool released{ false }; // pressed first time
        };

#define BUTTON_DECL(button, unused) ButtonState button

        FOR_EACH_BUTTON(BUTTON_DECL);

#undef BUTTON_DECL
    };

    struct KeyboardEvent {
        uint32_t key_code{ 0 };
        KeyboardState::ButtonState state;
    };

    Input(HWND hWnd);
    ~Input();

    void handle_win_input(WPARAM wparam, LPARAM lparam);
    void process_win_input();
    void clear_after_process(); // call after components handling in game main loop

    const KeyboardState& keyboard() const;
    const MouseState& mouse() const;

private:
    MouseState mouse_state_;
    KeyboardState keyboard_state_;

    std::queue<KeyboardEvent> keyboard_event_queue_;
    std::queue<MouseState> mouse_event_queue_;
};
