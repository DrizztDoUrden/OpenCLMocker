#pragma once

namespace OpenCL
{
	class Retainable
	{
	public:
		void Retain() { ++references; }
		bool Release() { return --references == 0; }

	protected:
		Retainable() = default;

	private:
		int references = 1;
	};
}
