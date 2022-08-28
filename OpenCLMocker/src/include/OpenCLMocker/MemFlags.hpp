#pragma once

#include <OpenCLMocker/Enums.hpp>

#include <CL/cl.h>

namespace OpenCL
{
	class MemFlags final : public FlagsEnum<cl_mem_flags, MemFlags>
	{
		using Base = FlagsEnum<cl_mem_flags, MemFlags>;

	public:
		MemFlags(cl_mem_flags value = 0);

		bool Validate() const override;

		MemFlags GetKernelAccessFlags() const
		{
			return GetValue() & CL_MEM_READ_ONLY | GetValue() & CL_MEM_WRITE_ONLY | GetValue() & CL_MEM_READ_WRITE;
		}

		MemFlags GetHostAccessFlags() const
		{
			return GetValue() & CL_MEM_HOST_READ_ONLY | GetValue() & CL_MEM_HOST_WRITE_ONLY | GetValue() & CL_MEM_HOST_NO_ACCESS;
		}

	private:
		static cl_mem_flags ReplaceDefault(cl_mem_flags value);
	};

	class MemFlagsType final : public EnumType<cl_mem_flags>
	{
		using Base = EnumType<cl_mem_flags>;

	public:
		static const MemFlagsType& Instance();

	private:
		MemFlagsType();
	};
}
