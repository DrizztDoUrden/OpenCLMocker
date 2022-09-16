#pragma once

namespace OpenCL
{
	struct ObjectHandle;

	class Retainable
	{
	public:
		ObjectHandle* handle = nullptr;

		Retainable() = default;

		void Retain(ObjectHandle& handle);
		bool Release(ObjectHandle& handle);
		int GetReferenceCount(ObjectHandle& handle) const;
	};
}
