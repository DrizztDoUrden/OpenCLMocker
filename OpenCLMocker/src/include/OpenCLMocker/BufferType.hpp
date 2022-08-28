#pragma once

#include <OpenCLMocker/Enums.hpp>

#include <CL/cl.h>

namespace OpenCL
{
	class BufferType final : public Enum<cl_buffer_create_type, BufferType>
	{
		using Base = Enum<cl_buffer_create_type, BufferType>;

	public:
		BufferType(cl_buffer_create_type value = 0);
	};

	class BufferTypeType final : public EnumType<cl_buffer_create_type>
	{
		using Base = EnumType<cl_buffer_create_type>;

	public:
		static const BufferTypeType& Instance();

	private:
		BufferTypeType();
	};
}
