#pragma once

#include <Windows.h>
#include <cstdint>

class Input;

class Win final
{
private:
    HWND hWnd_{ NULL };
    Input* input_{ nullptr };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);
public:
    Win() = default;
    ~Win() = default;

    bool initialize(uint32_t, uint32_t);
    void run();
    void destroy();

    HWND window() const;
    Input* input() const;

    float screen_width() const;
    float screen_height() const;
};
