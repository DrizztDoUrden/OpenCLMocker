#include <CL/cl.h>

#include <cassert>
#include <iostream>
#include <vector>

void Validate(cl_int status)
{
    assert(status==CL_SUCCESS);
}

int main()
{
    auto platformsNumber = cl_uint{};
    Validate(clGetPlatformIDs(0, nullptr, &platformsNumber));

    auto platforms = std::vector<cl_platform_id>{};
    platforms.resize(platformsNumber);
    Validate(clGetPlatformIDs(platformsNumber, platforms.data(), nullptr));

    std::cout << "Platform count: " << platforms.size() << std::endl;

    for (auto platform: platforms)
    {
        auto deviceNum = cl_uint{};
        Validate(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceNum));

        auto devices = std::vector<cl_device_id>{};
        devices.resize(deviceNum);
        Validate(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, deviceNum, devices.data(), nullptr));

        std::cout << "Device count: " << devices.size() << std::endl;

        cl_int status = 0;
        cl_context_properties cps[3] = {
            CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform), 0 };
        auto ctx = clCreateContextFromType(cps, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &status);
        Validate(status);
    }
    return 0;
}
