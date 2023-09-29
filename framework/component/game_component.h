#pragma once

class GameComponent
{
public:
    GameComponent() = default;
    virtual ~GameComponent() = default;

    virtual void initialize() = 0;
    virtual void draw() = 0;
    virtual void imgui() = 0;
    virtual void reload() = 0;
    virtual void update() = 0;
    virtual void destroy_resources() = 0;
};
