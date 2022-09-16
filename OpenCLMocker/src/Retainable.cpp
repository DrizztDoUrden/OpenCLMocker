#include <OpenCLMocker/Retainable.hpp>

#include <OpenCLMocker/MapToCl.hpp>

namespace OpenCL
{
	void Retainable::Retain(ObjectHandle& handle) { ++handle.useCount; }
	bool Retainable::Release(ObjectHandle& handle) { return --handle.useCount <= 0; }
	int Retainable::GetReferenceCount(ObjectHandle& handle) const { return handle.useCount; }
}
