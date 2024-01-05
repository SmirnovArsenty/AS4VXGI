#include "material.h"
#include "core/game.h"
#include "render/render.h"

void Material::use()
{
    auto context = Game::inst()->render().context();
    if (sampler_ != nullptr) {
        sampler_->use(0);
    } else {
        assert(false);
        return;
    }
    if (diffuse_ != nullptr) {
        context->PSSetShaderResources(0, 1, &diffuse_->view());
        context->VSSetShaderResources(0, 1, &diffuse_->view());
        context->HSSetShaderResources(0, 1, &diffuse_->view());
        context->DSSetShaderResources(0, 1, &diffuse_->view());
        context->GSSetShaderResources(0, 1, &diffuse_->view());
    }
}
