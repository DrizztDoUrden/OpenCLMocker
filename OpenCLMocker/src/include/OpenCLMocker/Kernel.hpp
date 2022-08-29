#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <CL/cl.h>

#include <map>
#include <string>
#include <vector>

namespace OpenCL
{
	class Context;
	class Program;

	struct KernArg
	{
		std::vector<char> value;
	};

	class Kernel : public Object, public Retainable, private KernelValidation
	{
	public:
		Context* ctx;
		Program* program;
		std::string name;

		Kernel() = default;

		static bool Validate(const Kernel* kernel) { return kernel != nullptr && kernel->Object::Validate() && kernel->KernelValidation::Validate(); }

		void SetArg(cl_uint index, size_t size, const void* value);

	private:
		std::map<size_t, KernArg> args;
	};
}

MapToCl(OpenCL::Kernel, cl_kernel)
