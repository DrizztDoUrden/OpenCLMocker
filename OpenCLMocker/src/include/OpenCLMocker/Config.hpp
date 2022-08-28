#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>

#include <vector>
#include <string>

namespace OpenCL
{
    class DeviceConfig
    {
        ForbidCopy(DeviceConfig);
        DefaultMove(DeviceConfig);

    public:
        std::string name = "Fake Device";
        std::string version = "0.0.1";
        std::string driver = "0.0.1";

        DeviceConfig() = default;
    };

    class PlatformConfig
    {
        ForbidCopy(PlatformConfig);
        DefaultMove(PlatformConfig);

    public:
        std::vector<DeviceConfig> devices{ 1 };

        std::string name = "Fake Platform";
        std::string vendor = "GPSProlapse";
        std::string profile = "No profile";
        std::string version = "0.0.1";

        PlatformConfig() = default;
    };

    class Config
    {
        ForbidCopy(Config);
        DefaultMove(Config);

    public:
        std::vector<PlatformConfig> platforms{ 1 };

        Config() = default;
        Config(const std::string& path);
    };
}
