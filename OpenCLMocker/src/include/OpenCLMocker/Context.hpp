#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Platform.hpp>
#include <OpenCLMocker/Retainable.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <CL/cl.h>

#include <functional>
#include <string>
#include <vector>

namespace OpenCL
{
	class Context : public Object, public Retainable, private ContextValidation
	{
	public:
		bool interopUserSync = false;
		Platform* platform = nullptr;
		std::vector<Device*> devices;

		std::function<void(const std::string& errorInfo, const std::vector<uint8_t>& privateInfo)> errorCallback;

		Context() = default;

		static bool Validate(const Context* ctx) { return ctx != nullptr && ctx->Object::Validate() && ctx->ContextValidation::Validate(); }
	};
}

MapToCl(OpenCL::Context, cl_context)
