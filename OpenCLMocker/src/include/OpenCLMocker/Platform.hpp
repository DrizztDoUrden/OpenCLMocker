#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>

#include <CL/cl.h>

#include <string>
#include <vector>

namespace OpenCL
{
    class Platform
    {
        ForbidCopy(Platform);

    public:
        std::vector<class Device> devices;

        std::string name;
        std::string vendor;
        std::string profile;
        std::string version;

        Platform(const class PlatformConfig& cfg);

        static std::vector<Platform*>& Get();
    };
}

MapToCl(OpenCL::Platform, cl_platform_id)
