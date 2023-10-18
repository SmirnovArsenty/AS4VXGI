#include "as4vxgi.h"

void AS4VXGI_Component::initialize()
{
    model_trees_.push_back({});
    ModelTree& model = model_trees_.back();
    model.load("./resources/models/suzanne.fbx");
}

void AS4VXGI_Component::draw()
{

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

}
