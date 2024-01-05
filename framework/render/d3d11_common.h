#pragma once

#include <Windows.h>
#include <cassert>

#define D3D11_CHECK(command)                                                                                                    \
    do {                                                                                                                        \
        HRESULT status = command;                                                                                               \
        if (FAILED(status)) {                                                                                                   \
            std::string info;                                                                                                   \
            info.resize(1000);                                                                                                  \
            sprintf_s(const_cast<char*>(info.data()), info.size(), "D3D11 error at " __FILE__ ":%d : %d", __LINE__, status);    \
            OutputDebugString(info.c_str());                                                                                    \
            MessageBox(NULL, info.c_str(), "OH NOOOOOOOO", MB_OK | MB_ICONERROR);                                               \
            assert(false);                                                                                                      \
        }                                                                                                                       \
    } while(0, 0)

#define SAFE_RELEASE(ptr)       \
    do {                        \
        if (ptr != nullptr) {   \
            (ptr)->Release();   \
        }                       \
        ptr = nullptr;          \
    } while (0, 0)
