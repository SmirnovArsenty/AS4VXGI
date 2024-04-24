#include <Windows.h>
#include "core/game.h"
#include "as4vxgi.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

int WINAPI WinMain(HINSTANCE, HINSTANCE, char*, int)
{
    // FILE* pix;
    // fopen_s(&pix, "WinPixGpuCapturer.dll", "r");
    // if (pix != nullptr) {
    //     fclose(pix);
    //     LoadLibrary("WinPixGpuCapturer.dll");
    // }

    Game::inst()->add_component(new AS4VXGI_Component{});

    Game::inst()->initialize(1280, 720);
    Game::inst()->run();
    Game::inst()->destroy();

    return 0;
}
