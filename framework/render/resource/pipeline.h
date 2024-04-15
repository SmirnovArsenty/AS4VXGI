#pragma once

#include "render/common.h"

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
    ComPtr<ID3D12RootSignature> root_signature_{ nullptr };
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptor_ranges_;

    ComPtr<ID3D12PipelineState> pso_{ nullptr };

    ComPtr<ID3D12GraphicsCommandList> command_list_{ nullptr };

    void create_root_signature();
public:
    virtual ~Pipeline();

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

    template<class bind>
    int resource_index()
    {
        bind info;
        for (int i = 0; i < descriptor_ranges_.size(); ++i) {
            if (descriptor_ranges_[i].BaseShaderRegister == info.slot() && descriptor_ranges_[i].RegisterSpace == info.space()) {
                return i;
            }
        }
        char error_msg[256];
        sprintf_s(error_msg, _countof(error_msg), "can't find registered range for current resource: %s slot: %d space: %d",
            std::is_same<decltype(info.type()), CBV>::value ? "cbv"
                                                            : (std::is_same<decltype(info.type()), SRV>::value ? "srv"
                                                                                                                : "uav"),
            info.slot(),
            info.space());
        OutputDebugString(error_msg);
        assert(false);
        return -1;
    }

    virtual void create_command_list() = 0;

    ID3D12GraphicsCommandList* add_cmd() const;
    ID3D12RootSignature* get_root_signature() const;
    ID3D12PipelineState* get_pso() const;
};

class GraphicsPipeline : public Pipeline
{
private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc_;

    ComPtr<IDxcBlob> pixel_shader_{ nullptr };
    ComPtr<IDxcBlob> vertex_shader_{ nullptr };
    ComPtr<IDxcBlob> geometry_shader_{ nullptr };

    ComPtr<ID3D12Resource> vertex_buffer_{ nullptr };
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_{};
public:
    GraphicsPipeline();
    ~GraphicsPipeline();

    void setup_depth_stencil_state(D3D12_DEPTH_STENCIL_DESC state);
    void setup_primitive_topology_type(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

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

    ComPtr<IDxcBlob> compute_shader_{ nullptr };
public:
    ComputePipeline();
    ~ComputePipeline();

    void attach_compute_shader(const std::wstring& path, const std::vector<std::wstring>& defines);

    void create_command_list() override;
};
