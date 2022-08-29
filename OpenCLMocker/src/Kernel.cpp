#include <OpenCLMocker/Kernel.hpp>

#include <OpenCLMocker/Exception.hpp>

#include <cstring>
#include <vector>

namespace OpenCL
{

	void Kernel::SetArg(cl_uint index, size_t size, const void* value)
	{
		if (index < 0) // Todo: upper limit check
			throw Exception(CL_INVALID_ARG_INDEX);

		auto& arg = args[index];

		arg.value.resize(size);
		std::memcpy(arg.value.data(), value, size);
	}

}
