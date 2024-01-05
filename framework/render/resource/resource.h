#pragma once

#include <string>

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void set_name(const std::string& name) = 0;
    virtual void destroy() = 0;
};
