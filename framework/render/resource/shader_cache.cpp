#include "shader_cache.h"

ShaderCache::ShaderCache()
{
}

ShaderCache::~ShaderCache()
{

}

Shader* ShaderCache::add_shader(const std::string& path, ShaderCache::ShaderFlags flags, D3D_SHADER_MACRO* macro)
{
    auto& shader_in_cache = shaders_.find(path);
    if (shader_in_cache != shaders_.end()) {
        return shader_in_cache->second; // already added
    }
    Shader* to_be_added = nullptr;
    if (flags == 0)
    { // add compute shader
        ComputeShader* s = new ComputeShader();
        s->set_compute_shader_from_file(path, "CSMain", macro);
        to_be_added = s;
    }
    else
    { // add graphics shader
        GraphicsShader* s = new GraphicsShader();
        if (flags & ShaderFlags::S_VERTEX) {
            s->set_vs_shader_from_file(path, "VSMain", macro);
        }
        if (flags & ShaderFlags::S_HULL) {
            //s->set_hs_shader_from_file(path, "HSMain", macro);
        }
        if (flags & ShaderFlags::S_DOMAIN) {
            // s->set_ds_shader_from_file(path, "DSMain", macro);
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

Shader* ShaderCache::get_shader(const std::string& path)
{
    return shaders_[path];
}

void ShaderCache::clear()
{
    for (auto& s : shaders_)
    {
        s.second->destroy();
        delete s.second;
        s.second = nullptr;
    }
    shaders_.clear();
}
