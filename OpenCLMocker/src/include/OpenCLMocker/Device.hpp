#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <CL/cl.h>

#include <string>

namespace OpenCL
{
    class Platform;

    class Device : public Object, private DeviceValidation
    {
    public:
        cl_device_type type = CL_DEVICE_TYPE_GPU;
        std::string name = "";
        std::string version = "";
        std::string driver = "";

        Device(Platform* platform) : _platform(platform) {}
        Device(Platform* platform, const class DeviceConfig& cfg);

        Platform* GetPlatform() const { return _platform; }

        static bool Validate(const Device* device) { return device != nullptr && device->Object::Validate() && device->DeviceValidation::Validate(); }

    private:
        Platform* _platform = nullptr;
    };
}

DirectMapToCl(OpenCL::Device, cl_device_id)
