#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <optional>
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
        friend void to_json(nlohmann::json& j, const Config& c);
        friend void from_json(const nlohmann::json& j, Config& c);

        std::vector<PlatformConfig> platforms{ 1 };
        std::optional<std::filesystem::path> dumpBuffersRoot;
        // Contains list of operations allowed to be dumped. None means any.
        std::vector<std::string> dumpBuffersOpFilter;

        Config() = default;

        static const Config& GetInstance();

    private:
        Config(const std::string& path);

        void OverrideFromEnvironment();
    };
}
