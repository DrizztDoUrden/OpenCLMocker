#include <OpenCLMocker/Platform.hpp>

#include <OpenCLMocker/Config.hpp>
#include <OpenCLMocker/Device.hpp>

namespace OpenCL
{
    static std::vector<Platform*> LoadPlatforms()
    {
        auto platforms = std::vector<Platform*>{};
        const auto cfg = Config{"/etc/mockcl.json"};

        for (const auto& pCfg : cfg.platforms)
            platforms.push_back(new Platform{ pCfg });

        return platforms;
    }

    Platform::Platform(const PlatformConfig& cfg)
        : name(cfg.name)
        , vendor(cfg.vendor)
        , profile(cfg.profile)
        , version(cfg.version)
    {
        for (const auto& dCfg : cfg.devices)
            devices.emplace_back(this, dCfg);
    }

    std::vector<Platform*>& Platform::Get()
    {
        static auto platforms = LoadPlatforms();
        return platforms;
    }
}
