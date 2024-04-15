#include "render/common.h"

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"

#include "as4vxgi.h"

#include <imgui/imgui.h>

int32_t voxel_grid_dim = 30;
float voxel_grid_size = 1000;

inline int align(int value, int alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void AS4VXGI_Component::initialize()
{
    Game::inst()->render().camera()->set_camera(Vector3(5, 0, 0), Vector3(-1, 0, 0));

    constexpr float offset = 300;
    constexpr float count = 10;
    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            model_trees_.push_back(new ModelTree{});
            model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3((x - count / 2) * offset, 0, (y - count / 2) * offset));
        }
    }

    // model_trees_.push_back(new ModelTree{});
    // model_trees_.back()->load("./resources/models/suzanne.fbx");//, Vector3(0, 0, 150));

    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3(0, 0, -150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/terrain.fbx");
    //model.load("./resources/models/sponza/source/sponza.fbx");

    auto device = Game::inst()->render().device();

    {
        if (true)
        {
            D3D12_INPUT_ELEMENT_DESC inputs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            stage_visualize_pipeline_.setup_input_layout(inputs, _countof(inputs));
            stage_visualize_pipeline_.attach_vertex_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_.attach_geometry_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_.attach_pixel_shader(L"./resources/shaders/voxels/draw.hlsl", {});

            // space 0
            stage_visualize_pipeline_.declare_bind<CAMERA_DATA_BIND>();

            // space 1
            stage_visualize_pipeline_.declare_bind<VOXELS_BIND>();
            stage_visualize_pipeline_.declare_bind<VOXEL_DATA_BIND>();

            CD3DX12_DEPTH_STENCIL_DESC ds_state(D3D12_DEFAULT);
            stage_visualize_pipeline_.setup_depth_stencil_state(ds_state);

            stage_visualize_pipeline_.setup_primitive_topology_type(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);

            stage_visualize_pipeline_.create_command_list();
        }
    }

    // create voxels uavs
    UINT voxels_uav_size = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim * sizeof(Voxel);
    {
        HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(uav_voxels_resource_.GetAddressOf())));

        HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(uav_voxels_resource_compute_.GetAddressOf())));

        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.Buffer.CounterOffsetInBytes = 0;
            uav_desc.Buffer.NumElements = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim;
            uav_desc.Buffer.StructureByteStride = sizeof(Voxel);
            uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            uav_voxels_resource_index_ = Game::inst()->render().allocate_resource_descriptor(uav_voxels_, uav_voxels_gpu_);
            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);

            uav_voxels_resource_index_compute_ = Game::inst()->render().allocate_resource_descriptor(uav_voxels_compute_, uav_voxels_gpu_compute_);
            device->CreateUnorderedAccessView(uav_voxels_resource_compute_.Get(), nullptr, &uav_desc, uav_voxels_compute_);
        }
    }

    // fill voxels
    for (int32_t i = 0; i < model_trees_.size(); ++i) {
        ModelTree* model_tree = model_trees_[i];
        Matrix transform = model_tree->get_transform();

        index_buffers_srv_.push_back({});
        for (D3D12_GPU_DESCRIPTOR_HANDLE srv : model_tree->get_index_buffers_srv()) {
            index_buffers_srv_[i].push_back(srv);
        }
        vertex_buffers_srv_.push_back({});
        for (D3D12_GPU_DESCRIPTOR_HANDLE srv : model_tree->get_vertex_buffers_srv()) {
            vertex_buffers_srv_[i].push_back(srv);
        }
        mesh_trees_srv_.push_back({});
        model_matrix_srv_.push_back({});
        for (std::vector<MeshTreeNode> mesh_tree : model_tree->get_meshes_trees()) {
            mesh_trees_srv_[i].push_back(new ShaderResource<MeshTreeNode>());
            mesh_trees_srv_[i].back()->initialize(mesh_tree.data(), UINT(mesh_tree.size()));

            model_matrix_srv_[i].push_back({});
            model_matrix_srv_[i].back() = new ShaderResource<Matrix>();
            model_matrix_srv_[i].back()->initialize(&transform, 1);
        }
    }
    voxel_data_.voxelGrid.dimension = voxel_grid_dim;
    voxel_data_.voxelGrid.size = voxel_grid_size;
    voxel_data_.voxelGrid.mesh_node_count = 0;

    // create const buffer view
    {
        voxel_data_cb_.initialize();
        voxel_data_cb_.update(voxel_data_);
    }

    {
        voxels_uav_usage_fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (voxels_uav_usage_fence_event_ == nullptr) {
            assert(!GetLastError());
        }

        HRESULT_CHECK(device->CreateFence(voxels_uav_usage_fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(voxels_uav_usage_fence_.ReleaseAndGetAddressOf())));
        voxels_uav_usage_fence_->SetName(L"Compute fence");
        ++voxels_uav_usage_fence_value_;
    }

    { // compute thread
        run_compute_thread_ = true;
        compute_thread_handle_ = CreateThread(nullptr, 0, &AS4VXGI_Component::compute_proc, this, 0, nullptr);
    }
}

void AS4VXGI_Component::draw()
{
    auto graphics_command_list_allocator = Game::inst()->render().graphics_command_allocator();
    auto resource_descriptor_heap = Game::inst()->render().resource_descriptor_heap();
    const auto render_target = Game::inst()->render().render_target();
    const auto depth_stencil_target = Game::inst()->render().depth_stencil();
    auto graphics_queue = Game::inst()->render().graphics_queue();
    if (true)
    {
        // visualize voxel grid
        {
            std::lock_guard<std::mutex> lg(uav_voxels_copy_mutex_);
            stage_visualize_pipeline_.cmd()->Reset(graphics_command_list_allocator.Get(), stage_visualize_pipeline_.get_pso());
            PIXBeginEvent(stage_visualize_pipeline_.cmd(), PIX_COLOR(0x0, 0xFF, 0x0), "Voxels draw");

            stage_visualize_pipeline_.cmd()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
            stage_visualize_pipeline_.cmd()->RSSetViewports(1, &Game::inst()->render().viewport());
            stage_visualize_pipeline_.cmd()->RSSetScissorRects(1, &Game::inst()->render().scissor_rect());
            stage_visualize_pipeline_.cmd()->OMSetRenderTargets(1, &render_target, 1, &depth_stencil_target);

            stage_visualize_pipeline_.cmd()->SetGraphicsRootSignature(stage_visualize_pipeline_.get_root_signature());
            stage_visualize_pipeline_.cmd()->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
            stage_visualize_pipeline_.cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());
            stage_visualize_pipeline_.cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<VOXELS_BIND>(), uav_voxels_gpu_);
            stage_visualize_pipeline_.cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<VOXEL_DATA_BIND>(), voxel_data_cb_.gpu_descriptor_handle());

            stage_visualize_pipeline_.cmd()->DrawInstanced(1, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 0, 0);

            PIXEndEvent(stage_visualize_pipeline_.cmd());
            HRESULT_CHECK(stage_visualize_pipeline_.cmd()->Close());

            ID3D12CommandList* lists[] = {stage_visualize_pipeline_.cmd()};
            graphics_queue->ExecuteCommandLists(_countof(lists), lists);

            PIXBeginEvent(graphics_queue.Get(), PIX_COLOR(0x0, 0xFF, 0x0), "sync");
            graphics_queue->Signal(voxels_uav_usage_fence_.Get(), voxels_uav_usage_fence_value_);

            HRESULT_CHECK(voxels_uav_usage_fence_->SetEventOnCompletion(voxels_uav_usage_fence_value_, voxels_uav_usage_fence_event_));
            if (voxels_uav_usage_fence_->GetCompletedValue() < voxels_uav_usage_fence_value_) {
                WaitForSingleObject(voxels_uav_usage_fence_event_, INFINITE);
            }
            ++voxels_uav_usage_fence_value_;
            PIXEndEvent(graphics_queue.Get());
        }
    }

    std::vector<ID3D12CommandList*> graphics_command_lists;
    for (ModelTree* model_tree : model_trees_) {
        std::vector<ID3D12CommandList*> current_cmd_lists = model_tree->draw(Game::inst()->render().camera());
        for (auto cmd : current_cmd_lists) {
            graphics_command_lists.push_back(cmd);
        }
    }
    graphics_queue->ExecuteCommandLists(static_cast<UINT>(graphics_command_lists.size()), graphics_command_lists.data());
}

void AS4VXGI_Component::imgui()
{
#ifndef NDEBUG
    // ImGuiViewport* main_viewport = ImGui::GetMainViewport();

    uint32_t window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 100), ImGuiCond_FirstUseEver);
    float local_voxel_grid_size = voxel_grid_size;
    int local_voxel_grid_dim = voxel_grid_dim;
    ImGui::Begin("info", nullptr, window_flags);
    {
        ImGui::Text("Voxel grid size");
        ImGui::SameLine();
        ImGui::InputFloat("##local_voxel_grid_size", &local_voxel_grid_size, 0.01f, 1.f);

        ImGui::Text("Voxel grid dimensions");
        ImGui::SameLine();
        ImGui::InputInt("##local_voxel_grid_dim", &local_voxel_grid_dim, 1, 10);
    }
    ImGui::End();

    if (local_voxel_grid_dim > 0 && local_voxel_grid_size > 0) {
        if (local_voxel_grid_dim != voxel_grid_dim) {
            std::lock_guard lg(uav_voxels_copy_mutex_);
            // uav_voxels_resource_.Reset();
            UINT voxels_uav_size = local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim * sizeof(Voxel);
            auto device = Game::inst()->render().device();

            HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
                D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(uav_voxels_resource_.ReleaseAndGetAddressOf())));

            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.Buffer.CounterOffsetInBytes = 0;
            uav_desc.Buffer.NumElements = local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim;
            uav_desc.Buffer.StructureByteStride = sizeof(Voxel);
            uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);
        }

        if (local_voxel_grid_size != voxel_grid_size || local_voxel_grid_dim != voxel_grid_dim) {
            voxel_grid_size = local_voxel_grid_size;
            voxel_grid_dim = local_voxel_grid_dim;

            voxel_data_.voxelGrid.dimension = voxel_grid_dim;
            voxel_data_.voxelGrid.size = voxel_grid_size;
            voxel_data_.voxelGrid.mesh_node_count = 0;
            voxel_data_cb_.update(voxel_data_);
        }
    }
#endif
}

void AS4VXGI_Component::reload()
{

}

void AS4VXGI_Component::update()
{

}

void AS4VXGI_Component::destroy_resources()
{
    run_compute_thread_ = false;
    WaitForSingleObject(compute_thread_handle_, 1000);
    CloseHandle(compute_thread_handle_);
    compute_thread_handle_ = INVALID_HANDLE_VALUE;

    //stage_visualize_pipeline_;

    SAFE_RELEASE(voxels_uav_usage_fence_);
    CloseHandle(voxels_uav_usage_fence_event_);

    model_matrix_srv_.clear();
    mesh_trees_srv_.clear();

    for (ModelTree* model_tree : model_trees_) {
        model_tree->unload();
        delete model_tree;
    }

    model_trees_.clear();

    SAFE_RELEASE(uav_voxels_resource_);
    SAFE_RELEASE(uav_voxels_resource_compute_);
}

// static
DWORD AS4VXGI_Component::compute_proc(LPVOID data)
{
    AS4VXGI_Component* self = static_cast<AS4VXGI_Component*>(data);

    auto device = Game::inst()->render().device();
    ID3D12CommandQueue* compute_queue;
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.NodeMask = 0;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        HRESULT_CHECK(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&compute_queue)));
        compute_queue->SetName(L"Voxels compute queue");
    }

    auto resource_heap = Game::inst()->render().resource_descriptor_heap();

    ID3D12CommandAllocator* allocator;
    HRESULT_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator)));
    allocator->SetName(L"Voxels command allocator");

    UINT64 compute_fence_value = 0;
    HANDLE compute_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (compute_fence_event == nullptr) {
        assert(!GetLastError());
    }

    ID3D12Fence* compute_fence;
    HRESULT_CHECK(device->CreateFence(compute_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&compute_fence)));
    compute_fence->SetName(L"Voxels fence");
    ++compute_fence_value;

    ComputePipeline clear_voxels;
    clear_voxels.attach_compute_shader(L"./resources/shaders/voxels/clear.hlsl", {});
    clear_voxels.declare_bind<VOXELS_BIND>();
    clear_voxels.create_command_list(D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator);

    ComputePipeline fill_voxels;
    fill_voxels.attach_compute_shader(L"./resources/shaders/voxels/fill.hlsl", {});
    fill_voxels.declare_bind<CAMERA_DATA_BIND>();
    fill_voxels.declare_bind<MESH_TREE_BIND>();
    fill_voxels.declare_bind<INDICES_BIND>();
    fill_voxels.declare_bind<VERTICES_BIND>();
    fill_voxels.declare_bind<MODEL_MATRICES_BIND>();
    fill_voxels.declare_bind<VOXEL_DATA_BIND>();
    fill_voxels.declare_bind<VOXELS_BIND>();
    fill_voxels.create_command_list(D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator);

    ComputePipeline copy_voxels;
    copy_voxels.attach_compute_shader(L"./resources/shaders/voxels/copy_voxels.hlsl", {});
    copy_voxels.declare_bind<VOXELS_SRC_BIND>();
    copy_voxels.declare_bind<VOXELS_DST_BIND>();
    copy_voxels.create_command_list(D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator);

    while (self->run_compute_thread_)
    {
        HRESULT_CHECK(allocator->Reset());

        {
            HRESULT_CHECK(clear_voxels.cmd()->Reset(allocator, clear_voxels.get_pso()));
            PIXBeginEvent(clear_voxels.cmd(), PIX_COLOR(0xFF, 0x0, 0x0), "Clear voxels");

            clear_voxels.cmd()->SetComputeRootSignature(clear_voxels.get_root_signature());
            clear_voxels.cmd()->SetDescriptorHeaps(1, resource_heap.GetAddressOf());
            clear_voxels.cmd()->SetComputeRootDescriptorTable(clear_voxels.resource_index<VOXELS_BIND>(), self->uav_voxels_gpu_compute_);
            clear_voxels.cmd()->Dispatch(align(voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 256) / 256, 1, 1);

            clear_voxels.cmd()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(self->uav_voxels_resource_compute_.Get()));

            PIXEndEvent(clear_voxels.cmd());
            HRESULT_CHECK(clear_voxels.cmd()->Close());
        }

        { // fill voxels uav
            fill_voxels.cmd()->Reset(allocator, fill_voxels.get_pso());
            PIXBeginEvent(fill_voxels.cmd(), PIX_COLOR(0xFF, 0x0, 0x0), "Voxels fill");

            fill_voxels.cmd()->SetComputeRootSignature(fill_voxels.get_root_signature());
            fill_voxels.cmd()->SetDescriptorHeaps(1, resource_heap.GetAddressOf());
            fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());

            fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<VOXEL_DATA_BIND>(), self->voxel_data_cb_.gpu_descriptor_handle());
            fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<VOXELS_BIND>(), self->uav_voxels_gpu_compute_);

            for (int i = 0; i < self->mesh_trees_srv_.size(); ++i) {
                for (int32_t j = 0; j < self->mesh_trees_srv_[i].size(); ++j) {
                    self->voxel_data_.voxelGrid.mesh_node_count = static_cast<int32_t>(self->mesh_trees_srv_[i][j]->size());
                    self->voxel_data_cb_.update(self->voxel_data_);

                    fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<MESH_TREE_BIND>(), self->mesh_trees_srv_[i][j]->gpu_descriptor_handle());
                    fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<INDICES_BIND>(), self->index_buffers_srv_[i][j]);
                    fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<VERTICES_BIND>(), self->vertex_buffers_srv_[i][j]);
                    fill_voxels.cmd()->SetComputeRootDescriptorTable(fill_voxels.resource_index<MODEL_MATRICES_BIND>(), self->model_matrix_srv_[i][j]->gpu_descriptor_handle());

                    fill_voxels.cmd()->Dispatch(align(voxel_grid_dim, 4) / 4,
                        align(voxel_grid_dim, 4) / 4,
                        align(voxel_grid_dim, 4) / 4);
                }
            }
            fill_voxels.cmd()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(self->uav_voxels_resource_compute_.Get()));

            PIXEndEvent(fill_voxels.cmd());
            HRESULT_CHECK(fill_voxels.cmd()->Close());
        }

        {
            std::lock_guard<std::mutex> lg(self->uav_voxels_copy_mutex_);

            { // copy voxels to render thread
                HRESULT_CHECK(copy_voxels.cmd()->Reset(allocator, copy_voxels.get_pso()));
                PIXBeginEvent(copy_voxels.cmd(), PIX_COLOR(0xFF, 0x0, 0x0), "Copy voxels");

                copy_voxels.cmd()->SetComputeRootSignature(copy_voxels.get_root_signature());
                copy_voxels.cmd()->SetDescriptorHeaps(1, resource_heap.GetAddressOf());
                copy_voxels.cmd()->SetComputeRootDescriptorTable(copy_voxels.resource_index<VOXELS_SRC_BIND>(), self->uav_voxels_gpu_compute_);
                copy_voxels.cmd()->SetComputeRootDescriptorTable(copy_voxels.resource_index<VOXELS_DST_BIND>(), self->uav_voxels_gpu_);
                copy_voxels.cmd()->Dispatch(align(voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 256) / 256, 1, 1);

                copy_voxels.cmd()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(self->uav_voxels_resource_.Get()));

                PIXEndEvent(copy_voxels.cmd());
                HRESULT_CHECK(copy_voxels.cmd()->Close());
            }

            ID3D12CommandList* cmd_lists[] = {clear_voxels.cmd(), fill_voxels.cmd(), copy_voxels.cmd()};
            compute_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);

            PIXBeginEvent(compute_queue, PIX_COLOR(0xFF, 0x0, 0x0), "sync");
            HRESULT_CHECK(compute_queue->Signal(compute_fence, compute_fence_value));
            HRESULT_CHECK(compute_fence->SetEventOnCompletion(compute_fence_value, compute_fence_event));
            if (compute_fence->GetCompletedValue() < compute_fence_value) {
                WaitForSingleObject(compute_fence_event, INFINITE);
            }
            ++compute_fence_value;
            PIXEndEvent(compute_queue);
        }
    }

    CloseHandle(compute_fence_event);

    compute_fence->Release();
    allocator->Release();
    compute_queue->Release();

    return 0;
}
