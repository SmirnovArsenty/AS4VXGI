#pragma once

#include <cstdint>
#include <memory>
#include <unordered_set>

class Win;
class Render;
class GameComponent;
class Scene;

class Game
{
private:
    std::unique_ptr<Win> win_;
    std::unique_ptr<Render> render_;
    std::unique_ptr<Scene> scene_;

    float delta_time_{ 0.f };

    bool destroy_{ false };
    bool animating_{ false };
    bool fullscreen_{ false };

    std::unordered_set<GameComponent*> game_components_;

    Game();
    Game(Game&) = delete;
    Game(const Game&&) = delete;
public:
    static Game* inst();
    ~Game();

    virtual void add_component(GameComponent*);

    virtual bool initialize(uint32_t, uint32_t);
    virtual void run();
    virtual void destroy();

    float delta_time() const;

    void set_animating(bool);
    void set_destroy();
    void resize();
    void toggle_fullscreen();

    const Win& win() const;
    const Render& render() const;
    Scene& scene() const;
};
