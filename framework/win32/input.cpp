#include <Windows.h>
#include <hidusage.h>
#include <stdexcept>
#include <string>
#include "input.h"
#include "core/game.h"

Input::Input(HWND hWnd) {
    RAWINPUTDEVICE raw_input_devices[2];
    raw_input_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_input_devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_input_devices[0].dwFlags = 0; // RIDEV_NOLEGACY ?
    raw_input_devices[0].hwndTarget = hWnd;

    raw_input_devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_input_devices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    raw_input_devices[1].dwFlags = 0; // RIDEV_NOLEGACY ?
    raw_input_devices[1].hwndTarget = hWnd;

    // HID_USAGE_GENERIC_GAMEPAD ?

    if (!RegisterRawInputDevices(raw_input_devices, static_cast<UINT>(std::size(raw_input_devices)), sizeof(RAWINPUTDEVICE)))
    {
        OutputDebugString(("Can not register raw input device. Error: " + std::to_string(GetLastError())).c_str());
        throw std::runtime_error("Can not register raw input device");
    }
}

Input::~Input() {}

void Input::handle_win_input(WPARAM wparam, LPARAM lparam)
{
    UINT dwSize;
    GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    BYTE* buffer = (BYTE*)alloca(dwSize);
    if (!buffer)
    {
        throw std::runtime_error("Can not allocate memory buffer");
    }

    if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
    {
        throw std::runtime_error("Error getting raw input data: wrong size");
    }

    RAWINPUT* raw_input = (RAWINPUT*)buffer;
    if (raw_input->header.dwType == RIM_TYPEKEYBOARD)
    {
        RAWKEYBOARD keyboard = raw_input->data.keyboard;

        KeyboardState::ButtonState button_state;
        button_state.pressed = (keyboard.Message == WM_KEYDOWN);
        button_state.released = (keyboard.Message == WM_KEYUP);

        keyboard_event_queue_.push({ keyboard.VKey, button_state });
    }
    else if (raw_input->header.dwType == RIM_TYPEMOUSE)
    {
        RAWMOUSE mouse = raw_input->data.mouse;
        MouseState mouse_state;
        {
            if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                mouse_state.lbutton.pressed = true;
            } else if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
                mouse_state.lbutton.released = true;
            }

            if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
                mouse_state.mbutton.pressed = true;
            } else if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
                mouse_state.mbutton.released = true;
            }

            if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
                mouse_state.rbutton.pressed = true;
            } else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
                mouse_state.rbutton.released = true;
            }

            mouse_state.delta_x = float(mouse.lLastX);
            mouse_state.delta_y = float(mouse.lLastY);
        }

        mouse_event_queue_.push(mouse_state);
    }

    DefRawInputProc(&raw_input, 1, sizeof(RAWINPUTHEADER));
}

void Input::process_win_input()
{
    if (!keyboard_event_queue_.empty())
    {
        KeyboardEvent keyboard_event = keyboard_event_queue_.front();
        keyboard_event_queue_.pop();
        if (keyboard_event.key_code == VK_ESCAPE) {
            Game::inst()->set_animating(false);
            Game::inst()->set_destroy();
        }

#define HANDLE_INPUT_EVENT(key, code)                                       \
    do {                                                                    \
        if (keyboard_event.key_code == code) {                              \
            keyboard_state_.key.pressed = keyboard_event.state.pressed;     \
            keyboard_state_.key.released = keyboard_event.state.released;   \
        }                                                                   \
    } while ((void)0, 0)
        FOR_EACH_BUTTON(HANDLE_INPUT_EVENT);
#undef HANDLE_INPUT_EVENT
    }

    while (!mouse_event_queue_.empty())
    {
        MouseState mouse_state = mouse_event_queue_.front();
        mouse_event_queue_.pop();
        mouse_state_.delta_x += mouse_state.delta_x;
        mouse_state_.delta_y += mouse_state.delta_y;
        mouse_state_.delta_z += mouse_state.delta_z;

        // lbutton
        if (mouse_state.lbutton.pressed) {
            mouse_state_.lbutton.pressed = true;
        }
        if (mouse_state.lbutton.released) {
            mouse_state_.lbutton.pressed = false;
            mouse_state_.lbutton.released = true;
        }

        // mbutton
        if (mouse_state.mbutton.pressed) {
            mouse_state_.mbutton.pressed = true;
        }
        if (mouse_state.mbutton.released) {
            mouse_state_.mbutton.pressed = false;
            mouse_state_.mbutton.released = true;
        }

        // rbutton
        if (mouse_state.rbutton.pressed) {
            mouse_state_.rbutton.pressed = true;
        }
        if (mouse_state.rbutton.released) {
            mouse_state_.rbutton.pressed = false;
            mouse_state_.rbutton.released = true;
        }
    }
}

void Input::clear_after_process()
{
    // clear mouse deltas
    mouse_state_.delta_x = 0;
    mouse_state_.delta_y = 0;
    mouse_state_.delta_z = 0;

    mouse_state_.lbutton.released = false;
    mouse_state_.mbutton.released = false;
    mouse_state_.rbutton.released = false;

    // clear keyboard release states
#define CLEAR_RELEASED(key, code)               \
    do {                                        \
        keyboard_state_.key.released = false;   \
    } while ((void)0, 0)

    FOR_EACH_BUTTON(CLEAR_RELEASED);

#undef CLEAR_RELEASED
}


const Input::MouseState& Input::mouse() const
{
    return mouse_state_;
}

const Input::KeyboardState& Input::keyboard() const
{
    return keyboard_state_;
}
