#pragma once

#include "resource.h"

#include "texture.h"
#include "sampler.h"

class Material
{
private:
    Texture2D* diffuse_{ nullptr };
    Sampler* sampler_{ nullptr };
public:
    Material() = default;
    ~Material()
    {
        assert(diffuse_ == nullptr);
    }

    void destroy()
    {
        if (sampler_ != nullptr) {
            sampler_->destroy();
            delete sampler_;
            sampler_ = nullptr;
        }
        if (diffuse_ != nullptr) {
            diffuse_->destroy();
            delete diffuse_;
            diffuse_ = nullptr;
        }
    }

    void use();

    void set_sampler(Sampler* sampler) { sampler_ = sampler; }

    void set_diffuse(Texture2D* diffuse) { diffuse_ = diffuse; }
};
