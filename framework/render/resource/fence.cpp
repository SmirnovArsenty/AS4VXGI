#include "core/game.h"
#include "render/render.h"
#include "fence.h"

void Fence::initialize()
{
    auto device = Game::inst()->render().device();

    D3D11_CHECK(device->CreateFence(0, D3D11_FENCE_FLAG_NONE, __uuidof(ID3D11Fence), (void**)&fence_));
}
