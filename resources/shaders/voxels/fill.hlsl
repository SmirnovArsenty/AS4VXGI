#define CAMERA_REGISTER b0
#define VOXEL_GRID_REGISTER b1
#include "resources/shaders/voxels/voxel.fx"

// b- slots

// u- slots
RWStructuredBuffer<Voxel> voxels : register(u0);

// t- slots
StructuredBuffer<MeshTreeNode> mesh_tree : register(t0);
StructuredBuffer<int> indices : register(t1);
StructuredBuffer<Vertex> vertices : register(t2);

struct Ray
{
    float3 origin;
    float3 direction;
};

Ray GenerateRay(uint3 voxel_location)
{
    Ray res;
    res.direction = camera.forward;
    res.origin = camera.position;
    res.origin -= camera.forward * voxel_grid.size / 2;

    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    res.origin -= right * voxel_grid.size / 2;

    up = cross(right, camera.forward);
    up = normalize(up);
    res.origin -= up * voxel_grid.size / 2;

    float unit = voxel_grid.size / voxel_grid.dimension;
    res.origin += unit * voxel_location.x * camera.forward;
    res.origin += unit * voxel_location.y * up;
    res.origin += unit * voxel_location.z * right;

    return res;
}

bool IsIntersect(Ray ray, MeshTreeNode mesh_node, out float t)
{
    t = 0;
    if (ray.direction.x == 0)
        ray.direction.x = 0.0001f;
    float tmin = (mesh_node.min.x - ray.origin.x) / ray.direction.x;
    float tmax = (mesh_node.max.x - ray.origin.x) / ray.direction.x;

    if (tmin > tmax) {
        float tmp = tmin;
        tmin = tmax;
        tmax = tmp;
    }

    if (ray.direction.y == 0)
        ray.direction.y = 0.0001f;
    float tymin = (mesh_node.min.y - ray.origin.y) / ray.direction.y;
    float tymax = (mesh_node.max.y - ray.origin.y) / ray.direction.y;

    if (tymin > tymax) {
        float tmp = tymin;
        tymin = tymax;
        tymax = tmp;
    }

    if ((tmin > tymax) || (tymin > tmax)) {
        return false;
    }

    if (tymin > tmin) {
        tmin = tymin;
    }

    if (tymax < tmax) {
        tmax = tymax;
    }

    if (ray.direction.z == 0)
        ray.direction.z = 0.0001f;
    float tzmin = (mesh_node.min.z - ray.origin.z) / ray.direction.z;
    float tzmax = (mesh_node.max.z - ray.origin.z) / ray.direction.z;

    if (tzmin > tzmax) {
        float tmp = tzmin;
        tzmin = tzmax;
        tzmax = tmp;
    }

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return false;
    }

    if (tzmin > tmin) {
        tmin = tzmin;
    }

    if (tzmax < tmax) {
        tmax = tzmax;
    }

    t = tmin;

    return true;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Voxel voxel = (Voxel)0;

    Ray ray = GenerateRay(dispatchThreadID);
    for (int i = 0; i < voxel_grid.mesh_node_count; ++i)
    {
        MeshTreeNode mesh_node = mesh_tree[i];
        float t;
        if (IsIntersect(ray, mesh_node, t)) {
            voxel.albedo = (t).xxx;
        }

    }

    voxels[dispatchThreadID.x + dispatchThreadID.y * 10 + dispatchThreadID.z * 10 * 10] = voxel;
}
