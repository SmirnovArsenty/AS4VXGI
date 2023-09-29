#pragma once

#include "core/game.h"
#include "render/render.h"
#include "render/resource/shader.h"

class SortLib
{
public:
    SortLib();
    virtual ~SortLib();

    void init();
    void run(unsigned int maxSize, ID3D11UnorderedAccessView* sortBufferUAV, ID3D11Buffer* itemCountBuffer);
    void release();

private:
    bool sort_initial( unsigned int maxSize );
    bool sort_incremental( unsigned int presorted, unsigned int maxSize );

private:

    ID3D11Buffer* dispatch_info_{ nullptr }; // constant buffer containing dispatch specific information

    ComputeShader sort_step_;
    ComputeShader sort512_;
    ComputeShader sort_inner512_;
    ComputeShader init_args_;

    ID3D11Buffer* indirect_args_{ nullptr };
    ID3D11UnorderedAccessView* indirect_args_UAV_{ nullptr };
};