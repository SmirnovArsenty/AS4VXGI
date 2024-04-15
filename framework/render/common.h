#pragma once

#include <Windows.h>
#include <cassert>

#include "headers/d3d12/directx/d3dx12.h"

#include "headers/WinPixEventRuntime/pix3.h"

#define HRESULT_CHECK(expr)                                                                                                                   \
    do {                                                                                                                                      \
        HRESULT status = expr;                                                                                                                \
        if (FAILED(status)) {                                                                                                                 \
            std::string info;                                                                                                                 \
            info.resize(1000);                                                                                                                \
            sprintf_s(const_cast<char*>(info.data()), info.size(), "HRESULT error at '%s': " __FILE__ ":%d : 0x%x", #expr, __LINE__, status); \
            OutputDebugString(info.c_str());                                                                                                  \
            MessageBox(NULL, info.c_str(), "OH NOOOOOOOO", MB_OK | MB_ICONERROR);                                                             \
            assert(false);                                                                                                                    \
        }                                                                                                                                     \
    } while(0, 0)

#define SAFE_RELEASE(com_ptr)     \
    do {                          \
        if (com_ptr != nullptr) { \
            com_ptr = nullptr;    \
        }                         \
    } while (0, 0)
