#pragma once

namespace OpenCL
{
	class Retainable
	{
	public:
		Retainable() = default;

		void Retain() { ++references; }
		bool Release() { return --references == 0; }
		int GetReferenceCount() const { return references; }

	private:
		int references = 1;
	};
}
