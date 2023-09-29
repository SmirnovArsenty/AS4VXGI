#pragma once

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

#include "render/resource/shader.h"
#include "render/resource/buffer.h"
#include "component/game_component.h"

#include "render/utils/sort_lib.h"

class ParticleSystem : public GameComponent
{
private:
    bool reset_flag_{ true };

    uint32_t max_particles_count_;

    inline int align( int value, int alignment ) {
        return ( value + (alignment - 1) ) & ~(alignment - 1);
    }

    inline float RandomVariance( float median, float variance ) {
        float fUnitRandomValue = (float)rand() / (float)RAND_MAX;
        float fRange = variance * fUnitRandomValue;
        return median - variance + (2.0f * fRange);
    }

    struct
    {
        Matrix view;
        Matrix view_inv;
        Matrix proj;
        Matrix proj_inv;
        Matrix view_proj;
        Matrix view_proj_inv;

        Vector3 camera_position;
        float screen_width;

        Vector3 camera_direction;
        float screen_height;

        float global_time;
        float frame_delta;
        Vector2 PerFrameConstBuffer_dummy;
    } uniform_data_;
    ConstBuffer uniform_buffer_;

    struct Particle
    {
        Vector3 position;
        float distance_to_camera_sqr;

        Vector3 velocity;
        float mass;
        Vector3 acceleration;
        float mass_delta;

        Vector4 start_color;
        Vector4 end_color;
        Vector4 color;

        float age;
        float life_span;
        float start_size;
        float end_size;
    };

    ID3D11Buffer* particle_pool_{ nullptr }; // all particles
    ID3D11ShaderResourceView* particle_pool_SRV_{ nullptr };
    ID3D11UnorderedAccessView* particle_pool_UAV_{ nullptr };

    // ID3D11Buffer* view_space_particle_positions_{ nullptr }; // cached particle positions in view space
    // ID3D11ShaderResourceView* view_space_particle_positions_SRV_{ nullptr };
    // ID3D11UnorderedAccessView* view_space_particle_positions_UAV_{ nullptr };

    ID3D11Buffer* dead_list_{ nullptr }; // dead particles indices
    ID3D11UnorderedAccessView* dead_list_UAV_{ nullptr };

    ID3D11Buffer* sort_list_{ nullptr }; // pair of particle index and distance (squared) to camera
    ID3D11ShaderResourceView* sort_list_SRV_{ nullptr };
    ID3D11UnorderedAccessView* sort_list_UAV_{ nullptr };

    ID3D11Buffer* index_buffer_{ nullptr };

    ID3D11Buffer* indirect_args_{ nullptr }; // args for indirect draw
    ID3D11UnorderedAccessView* indirect_args_UAV_{ nullptr };

    ID3D11Buffer* dead_list_const_buffer_{ nullptr };
    ID3D11Buffer* sort_list_const_buffer_{ nullptr };

    struct EmitterParams
    {
        Vector3 origin;
        uint32_t random;

        Vector3 velocity;
        float particle_life_span;

        float start_size;
        float end_size;
        float mass;
        int32_t max_particles_this_frame;

        Vector4 start_color;
        Vector4 end_color;
    } emitter_data_;
    ID3D11Buffer* emitter_const_buffer_{ nullptr };

    ID3D11BlendState* blend_state_{ nullptr };
    ID3D11DepthStencilState* depth_state_on_{ nullptr };
    ID3D11DepthStencilState* depth_state_off_{ nullptr };
    ID3D11RasterizerState* rasterizer_state_{ nullptr };

    // compute shaders
    ComputeShader init_dead_list_;
    ComputeShader emit_;
    ComputeShader simulate_;
    ComputeShader reset_;

    GraphicsShader render_;

    SortLib sort_lib_;

    void init_dead_list();
    void emit();
    void simulate();
    void sort();

#ifndef NDEBUG
    ID3D11Buffer* debug_counter_buffer_{ nullptr };
    int read_counter(ID3D11UnorderedAccessView* uav);

    uint32_t dead_particles_on_init_{ 0 };
    uint32_t dead_particles_after_emit_{ 0 };
    uint32_t dead_particles_after_simulation_{ 0 };
    uint32_t sort_particles_after_simulation_{ 0 };
#endif

    // external textures
    ID3D11ShaderResourceView* depth_view_{ nullptr };
    ID3D11ShaderResourceView* normal_view_{ nullptr };
    bool use_depth_for_collisions_{ 0 };
    bool use_depth_test_{ 0 };

public:
    ParticleSystem(uint32_t max_particles, Vector3 origin);
    ~ParticleSystem();

    void initialize() override;
    void draw() override;
    void imgui() override;
    void reload() override {};
    void update() override;
    void destroy_resources() override;

    void set_depth_shader_resource_view(ID3D11ShaderResourceView* depth_view, ID3D11ShaderResourceView* normal);
};
