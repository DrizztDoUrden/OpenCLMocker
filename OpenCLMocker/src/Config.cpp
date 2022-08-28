#include <OpenCLMocker/Config.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>

using nlohmann::json;

#define TryParse(json, config, field) \
    do { \
        if (json.contains(#field)) \
            json.at(#field).get_to(config.field); \
    } while (false)

#define TryParseVector(json, config, field) \
    do { \
        if (json.contains(#field)) \
        { \
            config.field.clear(); \
            json.at(#field).get_to(config.field); \
        } \
    } while (false)

namespace OpenCL
{
    static void to_json(json& j, const DeviceConfig& c)
    {
        j = json{
            {"name", c.name},
            {"version", c.version},
            {"driver", c.driver},
        };
    }

    static void from_json(const json& j, DeviceConfig& c)
    {
        TryParse(j, c, name);
        TryParse(j, c, version);
        TryParse(j, c, driver);
    }

    static void to_json(json& j, const PlatformConfig& c)
    {
        j = json{
            {"devices", c.devices},
            {"name", c.name},
            {"vendor", c.vendor},
            {"profile", c.profile},
            {"version", c.version},
        };
    }

    static void from_json(const json& j, PlatformConfig& c)
    {
        TryParseVector(j, c, devices);
        TryParse(j, c, name);
        TryParse(j, c, version);
        TryParse(j, c, vendor);
        TryParse(j, c, profile);
    }

    static void to_json(json& j, const Config& c)
    {
        j = json{ {"platforms", c.platforms}, };
    }

    static void from_json(const json& j, Config& c)
    {
        TryParseVector(j, c, platforms);
    }

    Config::Config(const std::string& path)
    {
        auto file = std::ifstream{ path };

        if (file.bad())
        {
            *this = {};
            return;
        }

        try
        {
            json j;
            file >> j;
            *this = j.get<Config>();
        }
        catch (const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }
}
