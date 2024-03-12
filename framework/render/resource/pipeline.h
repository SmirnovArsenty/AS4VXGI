#pragma once

#include <string>
#include <wrl.h>
#include <dxcapi.h>
using namespace Microsoft::WRL;

#include "shaders/common/types.fx"

class Pipeline
{
private:
    void declare_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT slot, UINT space);
protected:
    ComPtr<ID3D12RootSignature> root_signature_;
    std::vector<D3D12_ROOT_PARAMETER1> root_signature_params_;

    ComPtr<ID3D12PipelineState> pso_;

    ComPtr<ID3D12GraphicsCommandList> command_list_;

    void create_root_signature();
public:
    template<class bind>
    void declare_bind()
    {
        bind info;
        if (std::is_same<decltype(info.type()), CBV>::value) {
            declare_range(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, info.slot(), info.space());
        } else if (std::is_same<decltype(info.type()), SRV>::value) {
            declare_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, info.slot(), info.space());
        } else if (std::is_same<decltype(info.type()), UAV>::value) {
            declare_range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, info.slot(), info.space());
        }
    }

    virtual void create_command_list() = 0;

    ID3D12GraphicsCommandList* add_cmd();
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
    ~GraphicsPipeline();

    void setup_input_layout(const D3D12_INPUT_ELEMENT_DESC* inputElementDescs, uint32_t size);
    void attach_vertex_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    void attach_geometry_shader(const std::wstring& path, const std::vector<std::wstring>& defines);
    void attach_pixel_shader(const std::wstring& path, const std::vector<std::wstring>& defines);

    void create_command_list() override;
};

class ComputePipeline : public Pipeline
{
private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc_;

    ComPtr<IDxcBlob> compute_shader_;
public:
    ComputePipeline();
    ~ComputePipeline();

    void attach_compute_shader(const std::wstring& path, const std::vector<std::wstring>& defines);

    void create_command_list() override;
};
