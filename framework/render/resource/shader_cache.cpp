// #include "shader_cache.h"

ShaderCache::ShaderCache()
{
}

ShaderCache::~ShaderCache()
{

}

ComputeShader* ShaderCache::add_compute_shader(const std::string& path, D3D_SHADER_MACRO* macro)
{
    auto& shader_in_cache = shaders_.find(path);
    if (shader_in_cache != shaders_.end()) {
        return static_cast<ComputeShader*>(shader_in_cache->second); // already added
    }
    ComputeShader* s = new ComputeShader();
    s->set_compute_shader_from_file(path, "CSMain", macro);
    shaders_[path] = s;
    return s;
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
