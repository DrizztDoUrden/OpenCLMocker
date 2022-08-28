#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/MemFlags.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

#include <memory>

namespace OpenCL
{
	class Buffer : public Object, public Retainable, private BufferValidation
	{
	public:
		std::unique_ptr<char[]> gpuMemory = nullptr;
		std::unique_ptr<char[]> hostMemory = nullptr;

		OpenCL::Context* ctx = nullptr;
		char* start = nullptr;
		char* hostPtr = nullptr;
		std::size_t size = 0;

		Buffer(MemFlags flags) : flags(std::move(flags)) {}

		const MemFlags& GetMemFlags() const { return flags; }

		static bool Validate(const Buffer* buffer) { return buffer != nullptr && buffer->Object::Validate() && buffer->BufferValidation::Validate(); }

	private:
		MemFlags flags;
	};
}

MapToCl(OpenCL::Buffer, cl_mem)
