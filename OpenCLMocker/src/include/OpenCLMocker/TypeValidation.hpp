#pragma once

#include <cstddef>
#include <type_traits>

namespace OpenCL
{
	template <class validationValue>
	class Validation
	{
	public:
		bool Validate() const { return validation == validationValue{}; }

	private:
		std::size_t validation = validationValue{};
	};

	using ObjectValidation   = Validation<std::integral_constant<std::size_t, 0x123456789AB0000>>;
	using PlatformValidation = Validation<std::integral_constant<std::size_t, 0x123456789AB0001>>;
	using ContextValidation  = Validation<std::integral_constant<std::size_t, 0x123456789AB0002>>;
	using DeviceValidation   = Validation<std::integral_constant<std::size_t, 0x123456789AB0003>>;
	using QueueValidation    = Validation<std::integral_constant<std::size_t, 0x123456789AB0004>>;
	using BufferValidation   = Validation<std::integral_constant<std::size_t, 0x123456789AB0005>>;
	using ProgramValidation  = Validation<std::integral_constant<std::size_t, 0x123456789AB0006>>;
	using KernelValidation   = Validation<std::integral_constant<std::size_t, 0x123456789AB0007>>;
	using EventValidation    = Validation<std::integral_constant<std::size_t, 0x123456789AB0008>>;
}
