#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <d3dcompiler.h>

#include "render/resource/buffer.h"

class Shader : public Resource
{
protected:
    std::unordered_map<uint32_t, Buffer*> buffers_{}; // bind slot / buffer pointer (b- slots)
    std::unordered_map<uint32_t, Buffer*> resources_{}; // bind slot / shader resource pointer (t- slots)
    /// TODO // samplers (s- slots)
public:
    Shader() = default;
    virtual ~Shader() = default;

    virtual void set_name(const std::string&) = 0;
    virtual void use() = 0;
    virtual void destroy() = 0;

    inline void attach_buffer(uint32_t bind_slot, Buffer* buffer)
    {
        assert(buffers_.find(bind_slot) == buffers_.end());
        buffers_[bind_slot] = buffer;
    }

    inline void attach_resource(uint32_t bind_slot, Buffer* buffer)
    {
        assert(resources_.find(bind_slot) == resources_.end());
        resources_[bind_slot] = buffer;
    }
};

class GraphicsShader : public Shader
{
private:
    IndexBuffer* index_buffer_{ nullptr };
    VertexBuffer* vertex_buffer_{ nullptr };

    ID3D11InputLayout* input_layout_{ nullptr };
    ID3D11VertexShader* vertex_shader_{ nullptr };
    ID3DBlob* vertex_bc_{ nullptr };
    ID3D11HullShader* hull_shader_{ nullptr };
    ID3DBlob* hull_bc_{ nullptr };
    ID3D11DomainShader* domain_shader_{ nullptr };
    ID3DBlob* domain_bc_{ nullptr };
    ID3D11GeometryShader* geometry_shader_{ nullptr };
    ID3DBlob* geometry_bc_{ nullptr };
    ID3D11PixelShader* pixel_shader_{ nullptr };
    ID3DBlob* pixel_bc_{ nullptr };
public:
    GraphicsShader();
    ~GraphicsShader();

    void set_name(const std::string& name) override;

    void set_input_layout(D3D11_INPUT_ELEMENT_DESC*, size_t);

    inline void attach_index_buffer(IndexBuffer* index_buffer)
    {
        assert(index_buffer_ == nullptr);
        index_buffer_ = index_buffer;
    }

    inline void attach_vertex_buffer(VertexBuffer* vertex_buffer)
    {
        assert(vertex_buffer_ == nullptr);
        vertex_buffer_ = vertex_buffer;
    }

    void set_vs_shader_from_file(const std::string& filename,
                                 const std::string& entrypoint,
                                 D3D_SHADER_MACRO* macro = nullptr,
                                 ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_ds_shader_from_file(const std::string& filename,
                                 const std::string& entrypoint,
                                 D3D_SHADER_MACRO* macro = nullptr,
                                 ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_hs_shader_from_file(const std::string& filename,
                                 const std::string& entrypoint,
                                 D3D_SHADER_MACRO* macro = nullptr,
                                 ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_gs_shader_from_file(const std::string& filename,
                                 const std::string& entrypoint,
                                 D3D_SHADER_MACRO* macro = nullptr,
                                 ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_ps_shader_from_file(const std::string& filename,
                                 const std::string& entrypoint,
                                 D3D_SHADER_MACRO* macro = nullptr,
                                 ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);

    void set_vs_shader_from_memory(const std::string& data,
                                   const std::string& entrypoint,
                                   D3D_SHADER_MACRO* macro = nullptr,
                                   ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_gs_shader_from_memory(const std::string& data,
                                   const std::string& entrypoint,
                                   D3D_SHADER_MACRO* macro = nullptr,
                                   ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);
    void set_ps_shader_from_memory(const std::string& data,
                                   const std::string& entrypoint,
                                   D3D_SHADER_MACRO* macro = nullptr,
                                   ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);

    void use() override;

    void destroy() override;
};

class ComputeShader : public Shader
{
private:
    std::unordered_map<uint32_t, Buffer*> uavs_{}; // bind slot / UAV (u- slots)

    ID3D11ComputeShader* compute_shader_{ nullptr };
    ID3DBlob* compute_bc_{ nullptr };

public:
    ComputeShader();
    ~ComputeShader();

    void set_name(const std::string& name) override;

    void set_compute_shader_from_file(const std::string& filename,
                                      const std::string& entrypoint,
                                      D3D_SHADER_MACRO* macro = nullptr,
                                      ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);

    void set_compute_shader_from_memory(const std::string& data,
                                        const std::string& entrypoint,
                                        D3D_SHADER_MACRO* macro = nullptr,
                                        ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE);

    inline void attach_uav(uint32_t bind_slot, Buffer* buffer)
    {
        assert(uavs_.find(bind_slot) == uavs_.end());
        uavs_[bind_slot] = buffer;
    }

    void use() override;

    void destroy() override;
};
