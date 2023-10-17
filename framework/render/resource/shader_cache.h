#pragma once

#include <unordered_map>
#include <string>
#include "shader.h"

class ShaderCache
{
public:
    enum ShaderFlags : uint32_t
    {
        S_COMPUTE = 0,
        INPUT_LAYOUT = 1 << 0,
        S_VERTEX = 1 << 1,
        S_HULL = 1 << 2,
        S_DOMAIN = 1 << 3,
        S_GEOMETRY = 1 << 4,
        S_PIXEL = 1 << 5
    };

    ShaderCache();
    ~ShaderCache();

    Shader* add_graphics_shader(const std::string& path, uint32_t flags, D3D_SHADER_MACRO* macro)
    {
        assert(flags != ShaderCache::S_COMPUTE);

        auto& shader_in_cache = shaders_.find(path);
        if (shader_in_cache != shaders_.end()) {
            return shader_in_cache->second; // already added
        }
        Shader* to_be_added = nullptr;
        { // add graphics shader
            GraphicsShader<VertexType, IndexType>* s = new GraphicsShader<VertexType, IndexType>();
            if (flags & ShaderFlags::S_VERTEX) {
                s->set_vs_shader_from_file(path, "VSMain", macro);
            }
            if (flags & ShaderFlags::S_HULL) {
                s->set_hs_shader_from_file(path, "HSMain", macro);
            }
            if (flags & ShaderFlags::S_DOMAIN) {
                s->set_ds_shader_from_file(path, "DSMain", macro);
            }
            if (flags & ShaderFlags::S_GEOMETRY) {
                s->set_gs_shader_from_file(path, "GSMain", macro);
            }
            if (flags & ShaderFlags::S_PIXEL) {
                s->set_ps_shader_from_file(path, "PSMain", macro);
            }
            to_be_added = s;
        }
        shaders_[path] = to_be_added;
        return to_be_added;
    }

    ComputeShader* add_compute_shader(const std::string&, D3D_SHADER_MACRO* macro = nullptr);

    Shader* get_shader(const std::string& shader);

    void clear();

private:
    std::unordered_map<std::string, Shader*> shaders_;
};
