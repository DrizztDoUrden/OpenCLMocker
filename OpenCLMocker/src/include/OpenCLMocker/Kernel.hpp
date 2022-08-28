#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <CL/cl.h>

#include <string>

namespace OpenCL
{
	class Context;
	class Program;

	class Kernel : public Object, public Retainable, private KernelValidation
	{
	public:
		Context* ctx;
		Program* program;
		std::string name;

		Kernel() = default;

		static bool Validate(const Kernel* kernel) { return kernel != nullptr && kernel->Object::Validate() && kernel->KernelValidation::Validate(); }
	};
}

MapToCl(OpenCL::Kernel, cl_kernel)
