#include <OpenCLMocker/BufferType.hpp>
#include <OpenCLMocker/MemFlags.hpp>

namespace OpenCL
{
	MemFlags::MemFlags(cl_mem_flags value) : Base(&MemFlagsType::Instance(), ReplaceDefault(value))
	{
	}
	
	bool MemFlags::Validate() const
	{
		const auto rwFlagsCount =
			HasFlags(CL_MEM_READ_ONLY) +
			HasFlags(CL_MEM_WRITE_ONLY) +
			HasFlags(CL_MEM_READ_WRITE);

		const auto rwHostFlagsCount =
			HasFlags(CL_MEM_HOST_READ_ONLY) +
			HasFlags(CL_MEM_HOST_WRITE_ONLY) +
			HasFlags(CL_MEM_HOST_NO_ACCESS);

		if (rwFlagsCount > 1 || rwHostFlagsCount > 1)
			return false;
		if (HasFlags(CL_MEM_USE_HOST_PTR) && HasFlags(CL_MEM_COPY_HOST_PTR))
			return false;
		if (HasFlags(CL_MEM_USE_HOST_PTR) && HasFlags(CL_MEM_ALLOC_HOST_PTR))
			return false;

		return true;
	}

	cl_mem_flags MemFlags::ReplaceDefault(cl_mem_flags value)
	{
		if (value == 0)
			return CL_MEM_READ_WRITE;
		return value;
	}

	const MemFlagsType& MemFlagsType::Instance()
	{
		static const auto instance = MemFlagsType{};
		return instance;
	}

	MemFlagsType::MemFlagsType()
		: Base({
			CL_MEM_READ_WRITE,
			CL_MEM_WRITE_ONLY,
			CL_MEM_READ_ONLY,
			CL_MEM_USE_HOST_PTR,
			CL_MEM_COPY_HOST_PTR,
			CL_MEM_ALLOC_HOST_PTR,

			// 1.2
			CL_MEM_HOST_WRITE_ONLY,
			CL_MEM_HOST_READ_ONLY,
			CL_MEM_HOST_NO_ACCESS,

			// 2.0
			CL_MEM_KERNEL_READ_AND_WRITE,
			})
	{
	}

	BufferType::BufferType(cl_buffer_create_type value) : Base(&BufferTypeType::Instance(), value)
	{
	}

	const BufferTypeType& BufferTypeType::Instance()
	{
		static const auto instance = BufferTypeType{};
		return instance;
	}

	BufferTypeType::BufferTypeType()
		: Base({
			CL_BUFFER_CREATE_TYPE_REGION,
			})
	{
	}
}
