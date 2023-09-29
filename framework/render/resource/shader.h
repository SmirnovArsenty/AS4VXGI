#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <d3dcompiler.h>

class Shader
{
public:
    Shader() = default;
    virtual ~Shader() = default;

    virtual void set_name(const std::string&) = 0;
    virtual void use() = 0;
    virtual void destroy() = 0;
};

class GraphicsShader : public Shader
{
private:
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

    void set_vs_shader_from_file(const std::string& filename,
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

    void use() override;

    void destroy() override;
};
