#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

namespace OpenCL
{
	class Kernel;

	enum class BuildStatus
	{
		None,
		Error,
		Success,
		InProgress,
	};

	class Program : public Object, public Retainable, private ProgramValidation
	{
	public:
		Context* ctx;
		std::vector<Device*> devices;
		std::vector<std::string> sources;
		std::vector<std::vector<char>> binaries;
		std::vector<Kernel*> kernels;
		std::vector<BuildStatus> buildStatuses;
		std::vector<std::string> buildLogs;
		std::string options;

		Program() = default;

		static bool Validate(const Program* program) { return program != nullptr && program->Object::Validate() && program->ProgramValidation::Validate(); }
	};
}

MapToCl(OpenCL::Program, cl_program)
