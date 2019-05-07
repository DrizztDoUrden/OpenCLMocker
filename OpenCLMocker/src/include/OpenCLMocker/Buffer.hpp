#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

namespace OpenCL
{
    class Buffer : public Retainable
    {
        ForbidCopy(Buffer);

	public:
		Buffer() = default;
    };
}

MapToCl(OpenCL::Buffer, cl_mem)
