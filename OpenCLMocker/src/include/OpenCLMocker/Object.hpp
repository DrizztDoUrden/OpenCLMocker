#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <memory>

namespace OpenCL
{
	class Object : private ObjectValidation
	{
		ForbidCopy(Object);

	public:
		Object() = default;
		DefaultMove(Object);

	protected:
		inline bool Validate() const { return ObjectValidation::Validate(); }
	};
}
