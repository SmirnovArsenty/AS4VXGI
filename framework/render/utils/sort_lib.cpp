#include "sort_lib.h"
#include "render/d3d11_common.h"

const int MAX_NUM_TG = 1024;//128; // max 128 * 512 elements = 64k elements
typedef struct SortConstants
{
    int x,y,z,w;
} int4;


SortLib::SortLib()
{
}

SortLib::~SortLib()
{
}

void SortLib::init()
{
    auto device = Game::inst()->render().device();

    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory( &cbDesc, sizeof(cbDesc) );
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof( int4 );
    D3D11_CHECK(device->CreateBuffer(&cbDesc, nullptr, &dispatch_info_));

    // Step sort shader
    sort_step_.set_compute_shader_from_file("./resources/shaders/sort/sort_step.hlsl", "CSMain", nullptr, nullptr);

    // Create inner sort shader
    D3D_SHADER_MACRO innerDefines[] = {"SORT_SIZE", "512", nullptr,0};
    sort_inner512_.set_compute_shader_from_file("./resources/shaders/sort/sort_inner.hlsl", "CSMain", innerDefines, nullptr);

    // create
    D3D_SHADER_MACRO cs512Defines[] = {"SORT_SIZE", "512", nullptr, 0};
    sort512_.set_compute_shader_from_file("./resources/shaders/sort/sort.hlsl", "CSMain", cs512Defines, nullptr);

    init_args_.set_compute_shader_from_file("./resources/shaders/sort/init_sort_args.hlsl", "CSMain", nullptr, nullptr);

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.ByteWidth = 4 * sizeof( UINT );
    desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    D3D11_CHECK(device->CreateBuffer(&desc, nullptr, &indirect_args_));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav;
    ZeroMemory(&uav, sizeof(uav));
    uav.Format = DXGI_FORMAT_R32_UINT;
    uav.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav.Buffer.FirstElement = 0;
    uav.Buffer.NumElements = 4;
    uav.Buffer.Flags = 0;
    D3D11_CHECK(device->CreateUnorderedAccessView(indirect_args_, &uav, &indirect_args_UAV_));
}

void SortLib::run(unsigned int maxSize, ID3D11UnorderedAccessView* sortBufferUAV, ID3D11Buffer* itemCountBuffer)
{
    auto context = Game::inst()->render().context();

    // Capture current state
    ID3D11UnorderedAccessView* prevUAV = nullptr;
    context->CSGetUnorderedAccessViews(0, 1, &prevUAV);

    ID3D11Buffer* prevCBs[] = { nullptr, nullptr };
    context->CSGetConstantBuffers(0, ARRAYSIZE(prevCBs), prevCBs);

    ID3D11Buffer* cbs[] = { itemCountBuffer, dispatch_info_ };
    context->CSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);

    // Write the indirect args to a UAV
    context->CSSetUnorderedAccessViews(0, 1, &indirect_args_UAV_, nullptr);

    init_args_.use();
    context->Dispatch(1, 1, 1);

    context->CSSetUnorderedAccessViews(0, 1, &sortBufferUAV, nullptr);

    bool bDone = sort_initial(maxSize);

    int presorted = 512;
    while (!bDone) {
        bDone = sort_incremental(presorted, maxSize);
        presorted *= 2;
    }

#ifdef _DEBUG
    // this leaks resources somehow. Haven't looked into it yet.
    //manualValidate(maxSize, pUAV );
#endif

    // Restore previous state
    context->CSSetUnorderedAccessViews(0, 1, &prevUAV, nullptr);
    context->CSSetConstantBuffers(0, ARRAYSIZE(prevCBs), prevCBs);

    if (prevUAV) {
        prevUAV->Release();
    }

    for (size_t i = 0; i < ARRAYSIZE(prevCBs); i++) {
        if (prevCBs[i]) {
            prevCBs[i]->Release();
        }
    }
}

void SortLib::release()
{
    SAFE_RELEASE(dispatch_info_);

    sort_step_.destroy();
    sort512_.destroy();
    sort_inner512_.destroy();
    init_args_.destroy();

    SAFE_RELEASE(indirect_args_UAV_);
    SAFE_RELEASE(indirect_args_);
}

bool SortLib::sort_initial( unsigned int maxSize )
{
    bool bDone = true;

    // calculate how many threads we'll require:
    //   we'll sort 512 elements per CU (threadgroupsize 256)
    //     maybe need to optimize this or make it changeable during init
    //     TGS=256 is a good intermediate value
    

    unsigned int numThreadGroups = ((maxSize - 1) >> 9) + 1;

    assert(numThreadGroups <= MAX_NUM_TG);

    if (numThreadGroups > 1) {
        bDone = false;
    }

    // sort all buffers of size 512 (and presort bigger ones)
    sort512_.use();
    Game::inst()->render().context()->DispatchIndirect( indirect_args_, 0 );

    return bDone;
}

bool SortLib::sort_incremental(unsigned int presorted, unsigned int maxSize)
{
    bool bDone = true;
    sort_step_.use();

    // prepare thread group description data
    unsigned int numThreadGroups = 0;

    if (maxSize > presorted)
    {
        if (maxSize>presorted * 2) {
            bDone = false;
        }

        unsigned int pow2 = presorted;
        while (pow2 < maxSize) {
            pow2 *= 2;
        }
        numThreadGroups = pow2 >> 9;
    }

    unsigned int nMergeSize = presorted * 2;
    for(unsigned int nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 256; nMergeSubSize = nMergeSubSize >> 1)
//    for( int nMergeSubSize=nMergeSize>>1; nMergeSubSize>0; nMergeSubSize=nMergeSubSize>>1 ) 
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        
        Game::inst()->render().context()->Map(dispatch_info_, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
        SortConstants* sc = (SortConstants*)MappedResource.pData;
        sc->x = nMergeSubSize;
        if (nMergeSubSize == nMergeSize >> 1) {
            sc->y = (2 * nMergeSubSize - 1);
            sc->z = -1;
        } else {
            sc->y = nMergeSubSize;
            sc->z = 1;
        }
        sc->w = 0;
        Game::inst()->render().context()->Unmap(dispatch_info_, 0);

        Game::inst()->render().context()->Dispatch(numThreadGroups, 1, 1);
    }

    sort_inner512_.use();
    Game::inst()->render().context()->Dispatch(numThreadGroups, 1, 1);

    return bDone;
}
