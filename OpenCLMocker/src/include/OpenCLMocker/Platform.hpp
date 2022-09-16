#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>

#include <CL/cl.h>

#include <string>
#include <vector>

namespace OpenCL
{
	class Platform : public Object, private PlatformValidation
	{
	public:
		std::vector<class Device> devices;

		std::string name;
		std::string vendor;
		std::string profile;
		std::string version;

		Platform(const class PlatformConfig& cfg);

		static std::vector<Platform*>& Get();

		static bool Validate(const Platform* platform) { return platform != nullptr && platform->Object::Validate() && platform->PlatformValidation::Validate(); }
	};
}

DirectMapToCl(OpenCL::Platform, cl_platform_id)
