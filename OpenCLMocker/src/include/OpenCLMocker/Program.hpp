#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

namespace OpenCL
{
    class Program : public Retainable
    {
        ForbidCopy(Program);

	public:
		Program() = default;
    };
}

MapToCl(OpenCL::Program, cl_program)
