#include <OpenCLMocker/Device.hpp>

#include <OpenCLMocker/Config.hpp>

#include <iostream>

namespace OpenCL
{
    Device::Device(Platform* platform, const DeviceConfig& cfg)
        : Device(platform)
    {
        name = cfg.name;
        driver = cfg.driver;
        version = cfg.version;
    }
}
