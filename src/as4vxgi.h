#pragma once

#include "component/game_component.h"
#include "math/model_tree.h"

class AS4VXGI_Component final : public GameComponent
{
public:
    AS4VXGI_Component() = default;
    virtual ~AS4VXGI_Component() = default;

    void initialize() override;
    void draw() override;
    void imgui() override;
    void reload() override;
    void update() override;
    void destroy_resources() override;
private:
    std::vector<ModelTree> model_trees_;
};