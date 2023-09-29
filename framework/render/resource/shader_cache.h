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

    Shader* add_shader(const std::string&, ShaderFlags, D3D_SHADER_MACRO* macro = nullptr);

    Shader* get_shader(const std::string& shader);

    void clear();

private:
    std::unordered_map<std::string, Shader*> shaders_;
};