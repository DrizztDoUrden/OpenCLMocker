#pragma once

#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

#include <vector>

namespace OpenCL
{
    class Context : public Retainable
    {
        ForbidCopy(Context);

    public:
        Platform* platform = nullptr;
        std::vector<Device*> devices;

		Context() = default;
    };
}

MapToCl(OpenCL::Context, cl_context)
