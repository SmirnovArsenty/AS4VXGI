#pragma once

#include <string>

#define MATERIALS(FUNC)     \
    FUNC(diffuse)           \
    FUNC(specular)          \
    FUNC(ambient)           \
    FUNC(emissive)          \
    FUNC(height)            \
    FUNC(normal)            \
    FUNC(shininess)         \
    FUNC(opacity)           \
    FUNC(displacement)      \
    FUNC(lightmap)          \
    FUNC(reflection)        \
    FUNC(base_color)        \
    FUNC(normal_camera)     \
    FUNC(emission_color)    \
    FUNC(metalness)         \
    FUNC(diffuse_roughness) \
    FUNC(ambient_occlusion)

class Material
{
public:
    Material(const std::string& path);
    ~Material();

    void initialize();

    void destroy();

    void bind();

    bool is_pbr();

#define DECL_MATERIAL_TYPE(material_type)           \
    void set_##material_type(class Texture*);       \
    const class Texture* get_##material_type() const;

    MATERIALS(DECL_MATERIAL_TYPE)

#undef DECL_MATERIAL_TYPE

private:
    std::string path_;

#define MATERIAL_TYPE_PRIVATE_DECL(material_type)   \
    class Texture* material_type##_{ nullptr };

    MATERIALS(MATERIAL_TYPE_PRIVATE_DECL)

#undef MATERIAL_TYPE_PRIVATE_DECL

    ID3D11SamplerState* sampler_state_{ nullptr };

    static Texture default_texture_;
};
