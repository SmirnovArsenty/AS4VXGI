#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"

#include "as4vxgi.h"

void AS4VXGI_Component::initialize()
{
    model_trees_.push_back({});
    ModelTree& model = model_trees_.back();
    model.load("./resources/models/suzanne.fbx");
}

void AS4VXGI_Component::draw()
{
    for (auto& model_tree : model_trees_) {
        model_tree.draw(Game::inst()->render().camera());
    }
}

void AS4VXGI_Component::imgui()
{

}

void AS4VXGI_Component::reload()
{

}

void AS4VXGI_Component::update()
{

}

void AS4VXGI_Component::destroy_resources()
{
    for (auto& model_tree : model_trees_) {
        model_tree.unload();
    }
    model_trees_.clear();
}
