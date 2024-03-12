#include <sstream>

#include "render/common.h"

#include "pipeline.h"
#include "core/game.h"
#include "render/render.h"

#include <d3d12shader.h>


#pragma comment(lib, "dxcompiler.lib")


void CompileShader(std::wstring path, const std::vector<std::wstring>& defines, const wchar_t* main, const wchar_t* version, _In_ REFIID iid, _COM_Outptr_opt_result_maybenull_ void** ppvObject)
{
    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    WCHAR cwd[256];
    GetCurrentDirectoryW(256, cwd);

    std::vector<LPCWSTR> args = {
        path.c_str(),
        L"-E", main,
        L"-T", version,
    #if !defined(NDEBUG)
        L"-Zs",
    #endif
        // L"-Fo", (path + L".bin").c_str(),
        // L"-Fd", (path + L".pdb").c_str(),
        // L"-Qstring_reflect"
    };
    for (size_t i = 0; i < defines.size(); ++i) {
        args.push_back(L"-D");
        args.push_back(defines[i].c_str());
    }

    ComPtr<IDxcBlobEncoding> source;
    utils->LoadFile(path.c_str(), nullptr, &source);
    DxcBuffer source_buffer;
    source_buffer.Ptr = source->GetBufferPointer();
    source_buffer.Size = source->GetBufferSize();
    source_buffer.Encoding = DXC_CP_ACP;

    ComPtr<IDxcResult> result;

    ComPtr<IDxcIncludeHandler> include_handler;
    utils->CreateDefaultIncludeHandler(include_handler.GetAddressOf());

    compiler->Compile(&source_buffer, args.data(), static_cast<UINT32>(args.size()), include_handler.Get(), IID_PPV_ARGS(&result));

    ComPtr<IDxcBlobUtf8> error;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error), nullptr);
    if (error != nullptr && error->GetStringLength() != 0) {
        OutputDebugString(error->GetStringPointer());
    }

    HRESULT hres;
    result->GetStatus(&hres);
    if (FAILED(hres)) {
        assert(false);
        return;
    }

    ComPtr<IDxcBlobUtf16> shader_name;
    result->GetOutput(DXC_OUT_OBJECT, iid, ppvObject, &shader_name);
}

#pragma region ================================================================================================Pipeline================================================================================================

void Pipeline::create_root_signature()
{
    auto device = Game::inst()->render().device();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data)))) {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init_1_1(static_cast<UINT>(root_signature_params_.size()), root_signature_params_.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT status = D3DX12SerializeVersionedRootSignature(&root_signature_desc, feature_data.HighestVersion, &signature, &error);
    if (FAILED(status))
    {
        OutputDebugString((char*)error->GetBufferPointer());
        assert(false);
    }
    HRESULT_CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(root_signature_.GetAddressOf())));
}

void Pipeline::declare_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT slot, UINT space)
{
    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(range_type, 1, slot, space, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 param;
    param.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

    root_signature_params_.push_back(param);
}

ID3D12GraphicsCommandList* Pipeline::add_cmd()
{
    return command_list_.Get();
}

#pragma endregion

#pragma region ================================================================================================Graphics================================================================================================

GraphicsPipeline::GraphicsPipeline()
{
    pso_desc_.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc_.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc_.DepthStencilState.DepthEnable = FALSE;
    pso_desc_.DepthStencilState.StencilEnable = FALSE;
    pso_desc_.SampleMask = UINT_MAX;
    pso_desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc_.NumRenderTargets = 1;
    pso_desc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    /// TODO: pso_desc_.DSVFormat = ;
    pso_desc_.SampleDesc.Count = 1;
}

GraphicsPipeline::~GraphicsPipeline()
{
    command_list_.Reset();
    pso_.Reset();

    pixel_shader_.Reset();
    vertex_shader_.Reset();
    geometry_shader_.Reset();

    vertex_buffer_.Reset();

    root_signature_.Reset();
}

void GraphicsPipeline::setup_input_layout(const D3D12_INPUT_ELEMENT_DESC* inputElementDescs, uint32_t size)
{
    pso_desc_.InputLayout = { inputElementDescs, size };
}

void GraphicsPipeline::attach_vertex_shader(const std::wstring& path, const std::vector<std::wstring>& defines)
{
    CompileShader(path, defines, L"VSMain", L"vs_6_0", IID_PPV_ARGS(vertex_shader_.ReleaseAndGetAddressOf()));

    pso_desc_.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_->GetBufferPointer(), vertex_shader_->GetBufferSize());
}

void GraphicsPipeline::attach_geometry_shader(const std::wstring& path, const std::vector<std::wstring>& defines)
{
    CompileShader(path, defines, L"GSMain", L"gs_6_0", IID_PPV_ARGS(geometry_shader_.ReleaseAndGetAddressOf()));

    pso_desc_.GS = CD3DX12_SHADER_BYTECODE(geometry_shader_->GetBufferPointer(), geometry_shader_->GetBufferSize());
}

void GraphicsPipeline::attach_pixel_shader(const std::wstring& path, const std::vector<std::wstring>& defines)
{
    CompileShader(path, defines, L"PSMain", L"ps_6_0", IID_PPV_ARGS(pixel_shader_.ReleaseAndGetAddressOf()));

    pso_desc_.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_->GetBufferPointer(), pixel_shader_->GetBufferSize());
}

void GraphicsPipeline::create_command_list()
{
    assert(command_list_.Get() == nullptr);

    auto& device = Game::inst()->render().device();
    auto& allocator = Game::inst()->render().graphics_command_allocator();

    create_root_signature();
    pso_desc_.pRootSignature = root_signature_.Get();
    HRESULT_CHECK(device->CreateGraphicsPipelineState(&pso_desc_, IID_PPV_ARGS(pso_.ReleaseAndGetAddressOf())));
    HRESULT_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), pso_.Get(), IID_PPV_ARGS(command_list_.ReleaseAndGetAddressOf())));
    HRESULT_CHECK(command_list_->Close());
}

#pragma endregion

#pragma region ================================================================================================Compute================================================================================================

ComputePipeline::ComputePipeline()
{
    pso_desc_.NodeMask = 0;
    pso_desc_.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ nullptr, 0 };
    pso_desc_.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

ComputePipeline::~ComputePipeline()
{
    command_list_.Reset();
    pso_.Reset();

    compute_shader_.Reset();

    root_signature_.Reset();
}

void ComputePipeline::attach_compute_shader(const std::wstring& path, const std::vector<std::wstring>& defines)
{
    CompileShader(path, defines, L"CSMain", L"cs_6_0", IID_PPV_ARGS(compute_shader_.ReleaseAndGetAddressOf()));

    pso_desc_.CS = CD3DX12_SHADER_BYTECODE(compute_shader_->GetBufferPointer(), compute_shader_->GetBufferSize());
}

void ComputePipeline::create_command_list()
{
    assert(command_list_.Get() == nullptr);

    auto& device = Game::inst()->render().device();
    auto& allocator = Game::inst()->render().compute_command_allocator();

    create_root_signature();
    pso_desc_.pRootSignature = root_signature_.Get();
    HRESULT_CHECK(device->CreateComputePipelineState(&pso_desc_, IID_PPV_ARGS(pso_.ReleaseAndGetAddressOf())));
    HRESULT_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator.Get(), pso_.Get(), IID_PPV_ARGS(command_list_.ReleaseAndGetAddressOf())));
    HRESULT_CHECK(command_list_->Close());
}

#pragma endregion