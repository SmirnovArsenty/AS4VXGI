#define CAMERA_REGISTER b0
#define VOXEL_GRID_REGISTER b1
#include "resources/shaders/voxels/voxel.fx"

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
    res.direction = normalize(camera.forward);
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
    res.origin += unit * voxel_location.x * right;
    res.origin += unit * voxel_location.y * up;
    res.origin += unit * voxel_location.z * camera.forward;

    return res;
}

#define EPSILON 0.0001
bool inBoxBounds(MeshTreeNode mesh_node, float3 position)
{
    bool inside = true;
    for(int i = 0; i < 3; i++) {
        inside = inside && (position[i] > mesh_node.min[i] - EPSILON);
        inside = inside && (position[i] < mesh_node.max[i] + EPSILON);
    }

    return inside;
}

float boxIntersection(Ray ray, MeshTreeNode mesh_node)
{
    float3 corner0 = mesh_node.min;
    float3 corner1 = mesh_node.max;

    float coeffs[6];
    coeffs[0] = (corner0.x - ray.origin.x) / (ray.direction.x);
    coeffs[1] = (corner0.y - ray.origin.y) / (ray.direction.y);
    coeffs[2] = (corner0.z - ray.origin.z) / (ray.direction.z);
    coeffs[3] = (corner1.x - ray.origin.x) / (ray.direction.x);
    coeffs[4] = (corner1.y - ray.origin.y) / (ray.direction.y);
    coeffs[5] = (corner1.z - ray.origin.z) / (ray.direction.z);

    float t = -1;
    [unroll]
    for (int i = 0; i < 6; ++i)
        t = (coeffs[i] >= 0 && inBoxBounds(mesh_node, ray.origin + ray.direction * coeffs[i])) ? coeffs[i] : t;

    return t;
}

// bool IsIntersect(Ray ray, MeshTreeNode mesh_node, out float t)
// {
//     t = 0;
//     if (ray.direction.x == 0)
//         ray.direction.x = 0.0001f;
//     if (ray.direction.y == 0)
//         ray.direction.y = 0.0001f;
//     if (ray.direction.z == 0)
//         ray.direction.z = 0.0001f;

//     ray.direction = normalize(ray.direction);

//     float tmin = (mesh_node.min.x - ray.origin.x) / ray.direction.x;
//     float tmax = (mesh_node.max.x - ray.origin.x) / ray.direction.x;
//     if (tmin > tmax) {
//         float tmp = tmin;
//         tmin = tmax;
//         tmax = tmp;
//     }

//     float tymin = (mesh_node.min.y - ray.origin.y) / ray.direction.y;
//     float tymax = (mesh_node.max.y - ray.origin.y) / ray.direction.y;
//     if (tymin > tymax) {
//         float tmp = tymin;
//         tymin = tymax;
//         tymax = tmp;
//     }

//     if ((tmin > tymax) || (tymin > tmax)) {
//         return false;
//     }

//     if (tymin > tmin) {
//         tmin = tymin;
//     }

//     if (tymax < tmax) {
//         tmax = tymax;
//     }

//     float tzmin = (mesh_node.min.z - ray.origin.z) / ray.direction.z;
//     float tzmax = (mesh_node.max.z - ray.origin.z) / ray.direction.z;
//     if (tzmin > tzmax) {
//         float tmp = tzmin;
//         tzmin = tzmax;
//         tzmax = tmp;
//     }

//     if ((tmin > tzmax) || (tzmin > tmax)) {
//         return false;
//     }

//     if (tzmin > tmin) {
//         tmin = tzmin;
//     }

//     if (tzmax < tmax) {
//         tmax = tzmax;
//     }

//     t = tmax > max(tmin, 0.0) ? tmin : -1;
//     if (t > voxel_grid.size / voxel_grid.dimension)
//     {
//         t = -1;
//     }

//     return true;
// }

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Voxel voxel = (Voxel)0;

    Ray ray = GenerateRay(dispatchThreadID);
    for (int i = 0; i < voxel_grid.mesh_node_count; ++i)
    {
        MeshTreeNode mesh_node = mesh_tree[i];
        float t = boxIntersection(ray, mesh_node);
        if (t > EPSILON && t < voxel_grid.size / voxel_grid.dimension + EPSILON) {
            voxel.albedo.x = t;
        }

    }

    voxels[dispatchThreadID.x + dispatchThreadID.y * voxel_grid.dimension + dispatchThreadID.z * voxel_grid.dimension * voxel_grid.dimension] = voxel;
}
