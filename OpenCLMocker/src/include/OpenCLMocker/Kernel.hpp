#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

#include <string>

namespace OpenCL
{
    class Kernel : public Retainable
    {
        ForbidCopy(Kernel);

    public:
        std::string name;

		Kernel() = default;
    };
}

MapToCl(OpenCL::Kernel, cl_kernel)
