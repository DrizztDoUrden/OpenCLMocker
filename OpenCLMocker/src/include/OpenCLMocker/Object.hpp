#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

namespace OpenCL
{
	class Object : private ObjectValidation
	{
		ForbidCopy(Object);
		DefaultMove(Object);

	public:
		Object() = default;

	protected:
		bool Validate() const { return ObjectValidation::Validate(); }
	};
}
