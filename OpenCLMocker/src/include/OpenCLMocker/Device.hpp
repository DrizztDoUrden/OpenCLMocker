#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>

#include <CL/cl.h>

#include <string>

namespace OpenCL
{
    class Platform;

    class Device
    {
        ForbidCopy(Device);

    public:
        cl_device_type type = CL_DEVICE_TYPE_GPU;
        std::string name = "";
        std::string version = "";
        std::string driver = "";

        Device(Platform* platform) : _platform(platform) {}
        Device(Platform* platform, const class DeviceConfig& cfg);

        Platform* GetPlatform() const { return _platform; }

    private:
        Platform* _platform = nullptr;
    };
}

MapToCl(OpenCL::Device, cl_device_id)
