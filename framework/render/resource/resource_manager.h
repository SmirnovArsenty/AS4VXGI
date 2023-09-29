#pragma once

#include <cstdint>
#include <vector>

class Resource
{
protected:
    uint64_t handle_;
};

class ResourceManager final
{
public:
    ResourceManager();
    ~ResourceManager();

    void add_resource();
private:
    std::vector<Resource*> resources_;
};
