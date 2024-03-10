#pragma once

#include <string>
#include <wrl.h>
#include <dxcapi.h>
using namespace Microsoft::WRL;

class Pipeline
{
protected:
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pso_;
    
    ComPtr<ID3D12GraphicsCommandList> command_list_;
public:
    virtual ID3D12PipelineState* get_pso() = 0;
};

class GraphicsPipeline : public Pipeline
{
private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc_;

    ComPtr<IDxcBlob> pixel_shader_;
    ComPtr<IDxcBlob> vertex_shader_;
    ComPtr<IDxcBlob> geometry_shader_;

    ComPtr<ID3D12Resource> vertex_buffer_{};
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_{};

public:
    GraphicsPipeline();

    void setup_input_layout(const D3D12_INPUT_ELEMENT_DESC* inputElementDescs, uint32_t size);
    void attach_vertex_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    void attach_geometry_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    void attach_pixel_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    ID3D12PipelineState* get_pso();
};

class ComputePipeline : public Pipeline
{
private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc_;

    ComPtr<IDxcBlob> compute_shader_;
public:
    ComputePipeline();

    void attach_compute_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    ID3D12PipelineState* get_pso();
};
